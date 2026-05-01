#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

#include <audio/audio-system.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <json/json.hpp>
#include <save-system.hpp>
#include <level-cache.hpp>

// This state shows how to use some of the abstractions we created to make a menu.
class Menustate: public our::State {

    // A meterial holding the menu shader and the menu texture to draw
    our::TexturedMaterial* menuMaterial;
    // A rectangle mesh on which the menu material will be drawn
    our::Mesh* rectangle;
    // A variable to record the time since the state is entered (it will be used for the fading effect).
    float time;
    // Selection marker icon
    our::Texture2D* markerIcon = nullptr;
    bool showSettings = false;
    int masterVolume = 100;
    float mouseSensitivity = 0.003f;
    bool isFullscreen = false;
    bool showLevelSelect = false;
    int selectedButton = 0;
    int selectedLevelIndex = 0;
    bool hasContinueSession = false;
    std::vector<std::string> levelNames;
    std::vector<std::string> levelConfigs;
    int unlockedLevels = 1;
    bool debugMode = false;

    enum class MenuAction : int {
        Play = 0,
        Continue = 1,
        Settings = 2,
        Exit = 3
    };

    std::vector<MenuAction> getActions() const {
        std::vector<MenuAction> actions = {MenuAction::Play};
        if (hasContinueSession) actions.push_back(MenuAction::Continue);
        actions.push_back(MenuAction::Settings);
        actions.push_back(MenuAction::Exit);
        return actions;
    }

    const char* menuActionLabel(MenuAction action) const {
        switch (action) {
            case MenuAction::Play: return "   PLAY   ";
            case MenuAction::Continue: return " CONTINUE ";
            case MenuAction::Settings: return " SETTINGS ";
            case MenuAction::Exit: return "   EXIT   ";
            default: return "UNKNOWN";
        }
    }

    void loadLevelListFromConfig() {
        levelNames.clear();
        levelConfigs.clear();
        auto& cfg = getApp()->getConfig();

        if (cfg.contains("level-select") && cfg["level-select"].is_object()) {
            auto& levelSelect = cfg["level-select"];
            if (levelSelect.contains("levels") && levelSelect["levels"].is_array()) {
                for (auto& item : levelSelect["levels"]) {
                    if (!item.is_object()) continue;
                    std::string configPath = item.value("config", "");
                    if (configPath.empty()) continue;
                    std::string displayName = item.value("name", configPath);
                    levelNames.push_back(displayName);
                    levelConfigs.push_back(configPath);
                }
            }
        }

        if (levelConfigs.empty()) {
            levelNames = {"Level 1", "Level 2", "Level 3"};
            levelConfigs = {
                "config/levels/level1.jsonc",
                "config/levels/level2.jsonc",
                "config/levels/level3.jsonc"
            };
        }
        if (selectedLevelIndex >= (int)levelConfigs.size()) selectedLevelIndex = 0;

        // Prefetch all level configs so they're instant when selected
        for (const auto& configPath : levelConfigs) {
            our::LevelCache::prefetch(configPath);
        }
    }

    void loadAndEnterLevel(const std::string& levelPath, bool fromContinue) {
        auto& cfg = getApp()->getConfig();
        const nlohmann::json* cached = our::LevelCache::get(levelPath);
        nlohmann::json level;
        
        if (cached) {
            level = *cached;
        } else {
            std::ifstream f(levelPath);
            if(f){
                try {
                    level = nlohmann::json::parse(f, nullptr, true, true);
                } catch(const std::exception& e) {
                    std::cerr << "Failed to parse level config " << levelPath << ": " << e.what() << std::endl;
                    return;
                }
            } else {
                std::cerr << "Could not open level config: " << levelPath << std::endl;
                return;
            }
        }

        if(level.contains("scene")) cfg["scene"] = level["scene"];
        if(level.contains("game")) cfg["game"] = level["game"];
        cfg["active-level-config"] = levelPath;
        if (!fromContinue) {
            cfg["continue-session"]["active"] = false;
            cfg["continue-session"]["resume-requested"] = false;
        } else {
            cfg["continue-session"]["resume-requested"] = true;
        }
        getApp()->changeState("loading");
    }

    void executeAction(MenuAction action) {
        auto& cfg = getApp()->getConfig();
        if (action == MenuAction::Play) {
            showLevelSelect = true;
            selectedLevelIndex = 0;
            return;
        }
        if (action == MenuAction::Continue) {
            if (cfg.contains("continue-session") && cfg["continue-session"].is_object() &&
                cfg["continue-session"].value("active", false)) {
                std::string levelPath = cfg["continue-session"].value("level-config", "");
                if (!levelPath.empty()) {
                    loadAndEnterLevel(levelPath, true);
                }
            }
            return;
        }
        if (action == MenuAction::Settings) {
            showSettings = !showSettings;
            return;
        }
        getApp()->close();
    }

    void onInitialize() override {
        // First, we create a material for the menu's background
        menuMaterial = new our::TexturedMaterial();
        // Here, we load the shader that will be used to draw the background
        menuMaterial->shader = new our::ShaderProgram();
        menuMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        menuMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        menuMaterial->shader->link();
        // Then we load the menu texture
        menuMaterial->texture = our::texture_utils::loadImage("assets/textures/menu_bg.png");
        // Initially, the menu material will be black, then it will fade in
        menuMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        // Then we create a rectangle whose top-left corner is at the origin and its size is 1x1.
        // Note that the texture coordinates at the origin is (0.0, 1.0) since we will use the 
        // projection matrix to make the origin at the the top-left corner of the screen.
        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        },{
            0, 1, 2, 2, 3, 0,
        });

        // Reset the time elapsed since the state is entered.
        time = 0;
        // Close settings panel when re-entering menu
        showSettings = false;
        showLevelSelect = false;
        selectedButton = 0;
        selectedLevelIndex = 0;

        // Load marker icon
        markerIcon = our::texture_utils::loadImage("assets/textures/coin_icon.png");

        // Load initial settings from config
        auto& cfg = getApp()->getConfig();
        masterVolume = (int)(our::AudioSystem::instance().getMasterVolume() * 100.0f);
        
        if (cfg.contains("game")) {
            mouseSensitivity = cfg["game"].value("mouseSensitivity", 0.003f);
        }
        if (cfg.contains("window")) {
            isFullscreen = cfg["window"].value("fullscreen", false);
        }
        hasContinueSession = cfg.contains("continue-session") &&
                             cfg["continue-session"].is_object() &&
                             cfg["continue-session"].value("active", false);
        
        unlockedLevels = our::SaveSystem::getUnlockedLevels();
        debugMode = our::SaveSystem::getDebugMode();
        loadLevelListFromConfig();

        // Start menu background music
        our::AudioSystem::instance().playMusic("assets/audio/menu.mp3");

        // Ensure cursor is visible in menu
        glfwSetInputMode(getApp()->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(showLevelSelect){
            const int levelsCount = (int)levelConfigs.size();
            if (levelsCount > 0) {
                int activeLevels = debugMode ? levelsCount : unlockedLevels;
                if (keyboard.justPressed(GLFW_KEY_DOWN)) {
                    selectedLevelIndex = (selectedLevelIndex + 1) % activeLevels;
                    our::AudioSystem::instance().playSound("assets/audio/buttonSelect.mp3");
                }
                if (keyboard.justPressed(GLFW_KEY_UP)) {
                    selectedLevelIndex = (selectedLevelIndex - 1 + activeLevels) % activeLevels;
                    our::AudioSystem::instance().playSound("assets/audio/buttonSelect.mp3");
                }
                if (keyboard.justPressed(GLFW_KEY_SPACE) || keyboard.justPressed(GLFW_KEY_ENTER)) {
                    loadAndEnterLevel(levelConfigs[selectedLevelIndex], false);
                }
            }
            if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
                showLevelSelect = false;
            }
        } else if(keyboard.justPressed(GLFW_KEY_SPACE) || keyboard.justPressed(GLFW_KEY_ENTER)){
            auto actions = getActions();
            if(!actions.empty()) executeAction(actions[selectedButton]);
        } else if(keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, exit the game
            getApp()->close();
        }

        if(!showLevelSelect){
            auto actions = getActions();
            const int actionCount = (int)actions.size();
            if (keyboard.justPressed(GLFW_KEY_DOWN) && actionCount > 0) {
                selectedButton = (selectedButton + 1) % actionCount;
                our::AudioSystem::instance().playSound("assets/audio/buttonSelect.mp3");
            }
            if (keyboard.justPressed(GLFW_KEY_UP) && actionCount > 0) {
                selectedButton = (selectedButton - 1 + actionCount) % actionCount;
                our::AudioSystem::instance().playSound("assets/audio/buttonSelect.mp3");
            }
        }

        // Get the framebuffer size to set the viewport and the create the projection matrix.
        glm::ivec2 size = getApp()->getFrameBufferSize();
        // Make sure the viewport covers the whole size of the framebuffer.
        glViewport(0, 0, size.x, size.y);

        // The view matrix is an identity (there is no camera that moves around).
        // The projection matrix applys an orthographic projection whose size is the framebuffer size in pixels
        // so that the we can define our object locations and sizes in pixels.
        // Note that the top is at 0.0 and the bottom is at the framebuffer height. This allows us to consider the top-left
        // corner of the window to be the origin which makes dealing with the mouse input easier. 
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        // The local to world (model) matrix of the background which is just a scaling matrix to make the menu cover the whole
        // window. Note that we defind the scale in pixels.
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        // First, we apply the fading effect.
        time += (float)deltaTime;
        menuMaterial->tint = glm::vec4(glm::smoothstep(0.00f, 2.00f, time));
        // Then we render the menu background
        // Notice that I don't clear the screen first, since I assume that the menu rectangle will draw over the whole
        // window anyway.
        menuMaterial->setup();
        menuMaterial->shader->set("transform", VP*M);
        rectangle->draw();
    }

    void onImmediateGui() override {
        auto& keyboard = getApp()->getKeyboard();
        ImGuiIO& io = ImGui::GetIO();
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;

        // ── Game Title ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.18f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("##Title", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(3.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        ImGui::Text("Aladdin: Nasira's Revenge");
        ImGui::PopStyleColor();
        ImGui::End();

        // ── Subtitle ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.30f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("##Subtitle", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
        ImGui::Text("The Streets of Agrabah");
        ImGui::PopStyleColor();
        ImGui::End();

        // ── Buttons ──
        if (!showSettings && !showLevelSelect) {
            float btnW = 220.0f;
            float btnH = 50.0f;
            ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(btnW + 100, 0));
            ImGui::Begin("##MenuButtons", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
            ImGui::SetWindowFontScale(2.0f);

            // Style buttons
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

            auto actions = getActions();
            for(int i = 0; i < (int)actions.size(); i++){
                auto action = actions[i];
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
                bool pushedBorder = false;
                if (selectedButton == i) {
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    pushedBorder = true;
                }
                if (ImGui::Button(menuActionLabel(action), ImVec2(btnW, btnH))) {
                    executeAction(action);
                }
                if (ImGui::IsItemHovered()) selectedButton = i;
                if (pushedBorder) ImGui::PopStyleColor();

                if ((ImGui::IsItemHovered() || selectedButton == i) && markerIcon) {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    float coinY = min.y + (btnH - 32.0f) * 0.5f;
                    ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)markerIcon->getOpenGLName(), ImVec2(max.x + 10.0f, coinY), ImVec2(max.x + 42.0f, coinY + 32.0f));
                }
                if (i + 1 < (int)actions.size()) ImGui::Spacing();
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::End();
        }

        if(showLevelSelect){
            float panelW = 420.0f;
            ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.58f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(panelW, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.12f, 0.88f));
            ImGui::Begin("##LevelSelectPanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SetWindowFontScale(1.6f);
            ImGui::Text("Choose Level");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SetWindowFontScale(1.2f);
            for (int i = 0; i < (int)levelNames.size(); i++) {
                bool isLocked = !debugMode && (i + 1 > unlockedLevels);

                if (isLocked) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));
                    std::string lockedName = levelNames[i] + " [LOCKED]";
                    ImGui::Button(lockedName.c_str(), ImVec2(panelW - 60.0f, 40.0f));
                    ImGui::PopStyleColor();
                } else {
                    bool chosen = (selectedLevelIndex == i);
                    if (chosen) {
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                    if (ImGui::Button(levelNames[i].c_str(), ImVec2(panelW - 60.0f, 40.0f))) {
                        selectedLevelIndex = i;
                        loadAndEnterLevel(levelConfigs[i], false);
                    }
                    if (ImGui::IsItemHovered()) selectedLevelIndex = i;
                    if ((chosen || ImGui::IsItemHovered()) && markerIcon) {
                        ImVec2 min = ImGui::GetItemRectMin();
                        ImVec2 max = ImGui::GetItemRectMax();
                        float coinY = min.y + (40.0f - 28.0f) * 0.5f;
                        ImGui::GetWindowDrawList()->AddImage(
                            (void*)(intptr_t)markerIcon->getOpenGLName(),
                            ImVec2(max.x + 8.0f, coinY),
                            ImVec2(max.x + 36.0f, coinY + 28.0f)
                        );
                    }
                    if (chosen) ImGui::PopStyleColor();
                }
                ImGui::Spacing();
            }

            if (ImGui::Button("BACK", ImVec2(120.0f, 35.0f))) {
                showLevelSelect = false;
            }

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        // ── Settings Panel ──
        if (showSettings) {
            float panelW = 380.0f;
            ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(panelW, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.12f, 0.88f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.2f, 0.5f));
            ImGui::Begin("##SettingsPanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

            // Title
            ImGui::SetWindowFontScale(1.8f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
            float titleW = ImGui::CalcTextSize("SETTINGS").x;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleW) * 0.5f);
            ImGui::Text("SETTINGS");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Sound section
            ImGui::SetWindowFontScale(1.4f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
            ImGui::Text("Sound");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Volume slider
            ImGui::SetWindowFontScale(1.2f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.1f, 0.25f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.15f, 0.35f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);

            ImGui::SetNextItemWidth(panelW - 40.0f);
            if (ImGui::SliderInt("##Volume", &masterVolume, 0, 100, "Volume: %d%%")) {
                our::AudioSystem::instance().setMasterVolume((float)masterVolume / 100.0f);
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Camera section
            ImGui::SetWindowFontScale(1.4f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
            ImGui::Text("Controls");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::SetWindowFontScale(1.2f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.1f, 0.25f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::SetNextItemWidth(panelW - 40.0f);
            if (ImGui::SliderFloat("##Sensitivity", &mouseSensitivity, 0.0001f, 0.01f, "Sensitivity: %.4f")) {
                getApp()->getConfig()["game"]["mouseSensitivity"] = mouseSensitivity;
            }
            ImGui::PopStyleColor(2);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Display section
            ImGui::SetWindowFontScale(1.4f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
            ImGui::Text("Display");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::SetWindowFontScale(1.2f);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.1f, 0.25f, 0.9f));
            if (ImGui::Checkbox(" Fullscreen Mode", &isFullscreen)) {
                GLFWwindow* window = getApp()->getWindow();
                if (isFullscreen) {
                    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                } else {
                    glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 720, 0);
                }
                getApp()->getConfig()["window"]["fullscreen"] = isFullscreen;
            }
            ImGui::Spacing();
            if (ImGui::Checkbox(" Debug Mode (Unlock All Levels)", &debugMode)) {
                our::SaveSystem::setDebugMode(debugMode);
            }
            ImGui::PopStyleColor(2);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Back button
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120.0f) * 0.5f);
            if (ImGui::Button("CLOSE", ImVec2(120, 35))) {
                showSettings = false;
            }

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }

        // ── Controls hint ──
        float hintY = h * 0.92f;
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, hintY), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("##Hint", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        if(showLevelSelect){
            ImGui::Text("Arrows: Choose Level | Enter/Click: Start | ESC: Back");
        } else {
            ImGui::Text("Arrows: Navigate | Enter/Click: Select | ESC: Exit");
        }
        ImGui::PopStyleColor();
        ImGui::End();
    }

    void onDestroy() override {
        // Stop menu music
        our::AudioSystem::instance().stopMusic();
        // Delete all the allocated resources
        if(markerIcon) delete markerIcon;
        delete rectangle;
        delete menuMaterial->texture;
        delete menuMaterial->shader;
        delete menuMaterial;
    }
};
