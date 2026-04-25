#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <components/aladdin-controller.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/aladdin-controller.hpp>
#include <systems/collectible.hpp>
#include <systems/hazard.hpp>
#include <systems/enemy.hpp>
#include <systems/checkpoint.hpp>
#include <systems/level-exit.hpp>
#include <physics/physics-system.hpp>
#include <asset-loader.hpp>
#include <systems/room-portal.hpp>
#include <systems/dialogue.hpp>
#include <systems/projectile.hpp>
#include <audio/audio-system.hpp>
#include <systems/animation-system.hpp>

#include <imgui.h>
#include <sstream>
#include <iomanip>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>

// This state manages the main gameplay: loads the Agrabah maze level,
// runs all ECS systems, and renders the HUD overlay with scoring.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::PhysicsSystem physicsSystem;
    bool physicsInitialized = false;
    our::AladdinControllerSystem aladdinController;
    our::CollectibleSystem collectibleSystem;
    our::HazardSystem hazardSystem;
    our::EnemySystem enemySystem;
    our::CheckpointSystem checkpointSystem;
    our::LevelExitSystem levelExitSystem;
    our::RoomPortalSystem roomPortalSystem;
    our::DialogueSystem dialogueSystem;
    our::ProjectileSystem projectileSystem;
    our::AnimationSystem animationSystem;

    // -- Game Scoring State --
    int coinsCollected = 0;
    int totalCoins = 0;
    int enemiesKilled = 0;
    int totalEnemies = 0;
    float elapsedTime = 0.0f;

    // -- Health --
    static constexpr int kDefaultMaxHealth = 100;

    // Star rating time thresholds (seconds)
    int time3Star = 60;
    int time2Star = 90;
    int time1Star = 120;

    // -- UI Textures --
    our::Texture2D* coinIcon = nullptr;
    our::Texture2D* heartIcon = nullptr;
    our::Texture2D* enemyIcon = nullptr;

    void onInitialize() override {
        // Same Playstate instance can persist across menu <-> play; clear portal blackout / stale entity pointers.
        roomPortalSystem.reset();
        dialogueSystem.reset();

        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        // Skip if they were already loaded by the LoadingState
        if(config.contains("assets") && 
           our::AssetLoader<our::Mesh>::empty() && 
           our::AssetLoader<our::ShaderProgram>::empty() &&
           our::AssetLoader<our::Texture2D>::empty()){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Initialize the player controller system
        aladdinController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        // Read game metadata from config (set by the maze generator)
        auto& appConfig = getApp()->getConfig();
        if(appConfig.contains("game")){
            auto& game = appConfig["game"];
            totalCoins = game.value("total_coins", 0);
            totalEnemies = game.value("total_enemies", 0);
            time3Star = game.value("time_3star", 60);
            time2Star = game.value("time_2star", 90);
            time1Star = game.value("time_1star", 120);
        }

        // Reset gameplay state
        coinsCollected = 0;
        enemiesKilled = 0;
        elapsedTime = 0.0f;

        // Load UI icons
        coinIcon = our::texture_utils::loadImage("assets/textures/coin_icon.png");
        heartIcon = our::texture_utils::loadImage("assets/textures/heart_icon.png");
        enemyIcon = our::texture_utils::loadImage("assets/textures/monkey.png");

        // Start background music
        our::AudioSystem::instance().playMusic("assets/audio/bg.wav");
    }

    our::AladdinControllerComponent* findAladdin() {
        for (auto entity : world.getEntities()) {
            if (auto* a = entity->getComponent<our::AladdinControllerComponent>()) return a;
        }
        return nullptr;
    }

    our::Entity* findAladdinEntity() {
        for (auto entity : world.getEntities()) {
            if (entity->getComponent<our::AladdinControllerComponent>()) return entity;
        }
        return nullptr;
    }

    void onDraw(double deltaTime) override {
        elapsedTime += (float)deltaTime;
        if(!physicsInitialized){
            physicsInitialized = physicsSystem.initialize();
            if(!physicsInitialized){
                getApp()->changeState("menu");
                return;
            }
        }

        // Victory / other application-state transitions from LevelExit
        {
            std::string nextAppState = levelExitSystem.getNextApplicationState();
            if(!nextAppState.empty()){
                levelExitSystem.clearNextApplicationState();
                if(nextAppState == "victory"){
                    // Determine the next level path for the "Continue" button
                    auto& appConfig = getApp()->getConfig();
                    
                    // Save gameplay statistics for the victory screen
                    appConfig["last-stats"] = {
                        {"coinsCollected", coinsCollected},
                        {"totalCoins", totalCoins},
                        {"enemiesKilled", enemiesKilled},
                        {"totalEnemies", totalEnemies},
                        {"stars", calculateStars()},
                        {"time", elapsedTime}
                    };

                    std::string currentLevel = appConfig.value("play-level-config", "config/levels/level1.jsonc");
                    
                    if(currentLevel == "config/levels/level1.jsonc") {
                        appConfig["next-level-config"] = "config/levels/level2.jsonc";
                    } else {
                        // If no more levels, return to menu
                        appConfig["next-level-config"] = "menu";
                    }

                    getApp()->changeState("victory");
                    return;
                }
            }
        }

        // Check for level transition
        std::string nextScene = levelExitSystem.getNextScene();
        if(!nextScene.empty()){
            levelExitSystem.clearNextScene();
            // Load the new scene config
            std::ifstream file_in(nextScene);
            if(file_in){
                nlohmann::json new_config = nlohmann::json::parse(file_in, nullptr, true, true);
                file_in.close();
                if(new_config.contains("scene")){
                    getApp()->getConfig()["scene"] = new_config["scene"];
                }
                if(new_config.contains("game")){
                    getApp()->getConfig()["game"] = new_config["game"];
                }
                getApp()->changeState("play");
                return;
            } else {
                std::cerr << "Failed to load next scene: " << nextScene << std::endl;
            }
        }

        roomPortalSystem.update(&world, (float)deltaTime);
        if(roomPortalSystem.inBlackout()){
            our::RoomPortalSystem::renderBlackFrame(getApp()->getFrameBufferSize());
            world.deleteMarkedEntities();
            return;
        }

        // Dialogue system gates gameplay — when a Genie meeting is playing
        // the world is frozen and the player can only advance the lines or
        // press ESC to skip.
        glm::vec3 playerPos{0.0f};
        if(auto* aladdinEntity = findAladdinEntity()){
            playerPos = glm::vec3(aladdinEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
        }
        // Capture "was active" BEFORE updating. If the player hits ESC to
        // skip the conversation, DialogueSystem::update() ends it in-place
        // (isActive() becomes false), and we must NOT let that same ESC
        // press fall through to the menu-return branch below.
        const bool dialogueFrozen = dialogueSystem.isActive();
        dialogueSystem.update(&world, getApp()->getKeyboard(), getApp()->getMouse(),
                              playerPos, (float)deltaTime);
        const bool dialogueStillActive = dialogueSystem.isActive();

        // Freeze gameplay this frame if a dialogue is (or just was) active —
        // this covers: dialogue in progress, dialogue just started this
        // frame, and dialogue just ended this frame (so the skip-ESC doesn't
        // also trigger a jump etc.).
        const bool freezeGameplay = dialogueFrozen || dialogueStillActive;
        if(!freezeGameplay){
            // Here, we just run a bunch of systems to control the world logic
            movementSystem.update(&world, (float)deltaTime);
            aladdinController.update(&world, &physicsSystem, (float)deltaTime);
            enemySystem.update(&world, &physicsSystem, (float)deltaTime);
            physicsSystem.update(&world, (float)deltaTime);
            projectileSystem.update(&world, &physicsSystem, (float)deltaTime);
            aladdinController.postPhysicsUpdate(&world, &physicsSystem, (float)deltaTime);
            cameraController.update(&world, (float)deltaTime);
            collectibleSystem.update(&world, &physicsSystem, (float)deltaTime);
            hazardSystem.update(&world, &physicsSystem, (float)deltaTime);
            checkpointSystem.update(&world, &physicsSystem);
            levelExitSystem.update(&world);
            // Advance all skeletal animations so finalBoneMatrices[] are ready for the renderer
            animationSystem.update(&world, (float)deltaTime);
        }
        // Always render so the frozen scene stays on screen behind the
        // dialogue box.
        renderer.render(&world, getApp()->getFrameBufferSize());

        // Remove entities marked for deletion at the end of the frame
        world.deleteMarkedEntities();

        // Sync local stats with Aladdin's persistent component stats for HUD/Victory
        if (auto* aladdin = findAladdin()) {
            coinsCollected = aladdin->coinCount;
            enemiesKilled = aladdin->enemiesKilled;
        }

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(!freezeGameplay && keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape key is pressed in this frame, go to the menu state.
            // NOTE: when a dialogue is active OR was just skipped this frame,
            // freezeGameplay is true — so ESC is consumed by DialogueSystem
            // (to skip the meeting) and never falls through to here.
            getApp()->changeState("menu");
        }

        if (!freezeGameplay && keyboard.justPressed(GLFW_KEY_V)) {
            if (auto* a = findAladdin()) {
                a->cameraMode = (a->cameraMode == our::AladdinCameraMode::ThirdPerson)
                    ? our::AladdinCameraMode::FirstPerson
                    : our::AladdinCameraMode::ThirdPerson;
            }
        }

#if !defined(NDEBUG)
        // Debug shortcuts (enabled only in non-release builds, and only
        // when a dialogue isn't blocking input).
        if(!freezeGameplay){
            if(keyboard.justPressed(GLFW_KEY_G)){
                getApp()->changeState("gameover");
            }
            if(keyboard.justPressed(GLFW_KEY_F10)){
                getApp()->changeState("victory");
            }
            if(keyboard.justPressed(GLFW_KEY_C)){
                if(coinsCollected < totalCoins) coinsCollected++;
            }
            if(keyboard.justPressed(GLFW_KEY_K)){
                if(enemiesKilled < totalEnemies) enemiesKilled++;
            }
        }
#endif
    }

    // Helper to calculate star rating
    int calculateStars() const {
        // Stars based on time
        int timeStars = 0;
        if (elapsedTime <= time3Star) timeStars = 3;
        else if (elapsedTime <= time2Star) timeStars = 2;
        else if (elapsedTime <= time1Star) timeStars = 1;

        // Bonus: all coins = +1 potential, all enemies = +1 potential
        // Final rating = min(time-based stars, 3) with bonuses
        float coinRatio = totalCoins > 0 ? (float)coinsCollected / totalCoins : 1.0f;
        float enemyRatio = totalEnemies > 0 ? (float)enemiesKilled / totalEnemies : 1.0f;

        // If you didn't collect enough or kill enough, cap the stars
        if (coinRatio < 0.5f) timeStars = std::min(timeStars, 1);
        if (coinRatio < 0.8f) timeStars = std::min(timeStars, 2);
        if (enemyRatio < 0.5f) timeStars = std::min(timeStars, 2);

        return std::max(0, timeStars);
    }

    // Helper to format time as MM:SS
    std::string formatTime(float seconds) const {
        int mins = (int)seconds / 60;
        int secs = (int)seconds % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << mins 
            << ":" << std::setfill('0') << std::setw(2) << secs;
        return oss.str();
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();
        float screenWidth = io.DisplaySize.x;
        float screenHeight = io.DisplaySize.y;
        our::AladdinControllerComponent* aladdin = findAladdin();

        const int hp = aladdin ? aladdin->health : 0;
        const int livesCount = aladdin ? aladdin->lives : 0;
        const float hpFrac = aladdin ? glm::clamp((float)aladdin->health / (float)kDefaultMaxHealth, 0.0f, 1.0f) : 0.0f;

        // Helper for consistent panel styling
        auto beginPanel = [](const char* id, ImVec2 pos, ImVec2 size) {
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowSize(size);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.45f));
            ImGui::Begin(id, nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize);
        };

        auto endPanel = []() {
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        };

        // ═══════════════════════════════════════════════════════
        //  TOP-LEFT: Coin Collection
        // ═══════════════════════════════════════════════════════
        beginPanel("##CoinsPanel", ImVec2(20, 15), ImVec2(280, 0));
        if (coinIcon) {
            ImGui::Image((void*)(intptr_t)coinIcon->getOpenGLName(), ImVec2(32, 32));
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        ImGui::SetWindowFontScale(1.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        ImGui::Text("GOLD COINS");
        ImGui::PopStyleColor();
        
        float progress = totalCoins > 0 ? (float)coinsCollected / totalCoins : 1.0f;
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.8f, 0.0f, 0.9f));
        ImGui::ProgressBar(progress, ImVec2(200, 15), ""); 
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Text("%d / %d Collected", coinsCollected, totalCoins);
        ImGui::EndGroup();
        endPanel();

        // ═══════════════════════════════════════════════════════
        //  TOP-CENTER: Timer & Stars
        // ═══════════════════════════════════════════════════════
        beginPanel("##TimerPanel", ImVec2(screenWidth / 2.0f - 85, 15), ImVec2(170, 0));
        ImGui::SetWindowFontScale(2.2f);
        ImVec4 timerColor;
        if (elapsedTime <= time3Star) timerColor = ImVec4(0.2f, 1.0f, 0.3f, 1.0f);
        else if (elapsedTime <= time2Star) timerColor = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
        else if (elapsedTime <= time1Star) timerColor = ImVec4(1.0f, 0.5f, 0.1f, 1.0f);
        else timerColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, timerColor);
        std::string timeStr = formatTime(elapsedTime);
        float textWidth = ImGui::CalcTextSize(timeStr.c_str()).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);
        ImGui::Text("%s", timeStr.c_str());
        ImGui::PopStyleColor();

        // Stars below timer
        ImGui::SetWindowFontScale(1.6f);
        int stars = calculateStars();
        float starsWidth = (1.6f * 15.0f) * 3; // Approx
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 70) * 0.5f);
        for (int i = 0; i < 3; i++) {
            if (i > 0) ImGui::SameLine(0, 4);
            if (i < stars) ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "*");
            else ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 0.5f), "*");
        }
        endPanel();

        // ═══════════════════════════════════════════════════════
        //  TOP-RIGHT: Enemies Killed
        // ═══════════════════════════════════════════════════════
        beginPanel("##EnemiesPanel", ImVec2(screenWidth - 300, 15), ImVec2(280, 0));
        if (enemyIcon) {
            ImGui::Image((void*)(intptr_t)enemyIcon->getOpenGLName(), ImVec2(32, 32));
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        ImGui::SetWindowFontScale(1.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("NASIRA'S GUARDS");
        ImGui::PopStyleColor();

        float enemyProgress = totalEnemies > 0 ? (float)enemiesKilled / totalEnemies : 1.0f;
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.1f, 0.1f, 0.9f));
        ImGui::ProgressBar(enemyProgress, ImVec2(200, 15), "");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Text("%d / %d Defeated", enemiesKilled, totalEnemies);
        ImGui::EndGroup();
        endPanel();

        // ═══════════════════════════════════════════════════════
        //  BOTTOM-LEFT: Lives & Health
        // ═══════════════════════════════════════════════════════
        beginPanel("##HealthPanel", ImVec2(20, screenHeight - 130), ImVec2(280, 0));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("LIVES: %d", livesCount);
        ImGui::Spacing();
        ImGui::Text("HEALTH");
        ImVec4 healthColor = ImVec4(1.0f - hpFrac, hpFrac, 0.15f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
        ImGui::ProgressBar(hpFrac, ImVec2(220, 14), "");
        ImGui::PopStyleColor();
        ImGui::Text("%d / %d", hp, kDefaultMaxHealth);
        ImGui::Spacing();
        if (heartIcon) {
            ImGui::Text(" ");
            ImGui::SameLine();
            for (int i = 0; i < livesCount && i < 8; i++) {
                if (i > 0) ImGui::SameLine(0, 6);
                ImGui::Image((void*)(intptr_t)heartIcon->getOpenGLName(), ImVec2(26, 26));
            }
        }
        endPanel();

        // ═══════════════════════════════════════════════════════
        //  BOTTOM-CENTER: Controls
        // ═══════════════════════════════════════════════════════
        ImGui::SetNextWindowPos(ImVec2(screenWidth / 2.0f - 250, screenHeight - 45));
        ImGui::Begin("##Controls", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.1f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.4f));
        ImGui::Text("WASD: Move | V: 1st/3rd cam | F10: win (dbg) | ESC: Menu");
        ImGui::PopStyleColor();
        ImGui::End();

        // ═══════════════════════════════════════════════════════
        //  AIMING CROSSHAIR
        // ═══════════════════════════════════════════════════════
        if (aladdin && aladdin->isAiming) {
            ImVec2 center = ImVec2(screenWidth * 0.5f + aladdin->aimOffset.x, screenHeight * 0.5f + aladdin->aimOffset.y);
            float size = 20.0f;
            float thickness = 2.0f;
            ImU32 color = IM_COL32(255, 255, 255, 220); 
            
            auto drawList = ImGui::GetForegroundDrawList();
            // Horizontal line
            drawList->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), color, thickness);
            // Vertical line
            drawList->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), color, thickness);
            // Center Dot
            drawList->AddCircleFilled(center, 3.0f, color);
            
            // Add a small shadow/outline to make it visible on bright backgrounds
            drawList->AddCircle(center, 3.5f, IM_COL32(0, 0, 0, 150), 12, 1.0f);
        }

        // For debugging only (TODO: remove or disable in production builds)
        aladdinController.onImmediateGui(&world);

        // Draw the Genie / Aladdin dialogue box last so it sits on top of
        // the HUD. No-op when no conversation is active.
        dialogueSystem.renderImGui();
    }

    void onDestroy() override {
        // Shutdown physics system and clear its bodies
        if(physicsInitialized){
            physicsSystem.shutdown(&world);
        }
        physicsInitialized = false;
        // Stop the background music for this level
        our::AudioSystem::instance().stopMusic();
        // Don't forget to destroy the renderer
        renderer.destroy();
        // Free-camera may have locked the cursor; exit restores normal cursor mode.
        cameraController.exit();
        roomPortalSystem.reset();
        // Clear the world
        world.clear();
        // Delete UI icons
        if(coinIcon) delete coinIcon;
        if(heartIcon) delete heartIcon;
        if(enemyIcon) delete enemyIcon;

        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};