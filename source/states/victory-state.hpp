#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <json/json.hpp>

// Victory State — displayed when the player reaches the end portal.
// Shows a celebratory Aladdin-themed background with fade-in effect.
// Press SPACE/ENTER to return to menu, ESC to quit.
class VictoryState : public our::State {

    our::TexturedMaterial* bgMaterial;
    our::Mesh* rectangle;
    float time;

    // UI Icons
    our::Texture2D* coinIcon = nullptr;
    our::Texture2D* enemyIcon = nullptr;

    void onInitialize() override {
        // Create the background material
        bgMaterial = new our::TexturedMaterial();
        bgMaterial->shader = new our::ShaderProgram();
        bgMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        bgMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        bgMaterial->shader->link();
        bgMaterial->texture = our::texture_utils::loadImage("assets/textures/victory_bg.jpg");
        bgMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Start black, fade in
        bgMaterial->alphaThreshold = 0.0f;

        // Fullscreen rectangle (same as menu-state pattern)
        rectangle = new our::Mesh({
            {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        }, {
            0, 1, 2, 2, 3, 0,
        });

        time = 0;

        // Load summary icons
        coinIcon = our::texture_utils::loadImage("assets/textures/coin_icon.png");
        enemyIcon = our::texture_utils::loadImage("assets/textures/monkey.png");

        // Ensure cursor is visible in victory screen
        glfwSetInputMode(getApp()->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void onDraw(double deltaTime) override {
        auto& keyboard = getApp()->getKeyboard();

        // SPACE or ENTER → return to menu
        if (keyboard.justPressed(GLFW_KEY_SPACE) || keyboard.justPressed(GLFW_KEY_ENTER)) {
            getApp()->changeState("menu");
        }
        // ESC → quit
        if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            getApp()->close();
        }

        // Setup orthographic projection
        glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        // Fade-in effect over 2 seconds (slightly slower for victory celebration feel)
        time += (float)deltaTime;
        bgMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 2.0f, time));

        // Draw fullscreen background
        bgMaterial->setup();
        bgMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }

    void onImmediateGui() override {
        auto& appConfig = getApp()->getConfig();
        auto stats = appConfig.value("last-stats", nlohmann::json::object());
        
        int coins = stats.value("coinsCollected", 0);
        int tCoins = stats.value("totalCoins", 0);
        int enemies = stats.value("enemiesKilled", 0);
        int tEnemies = stats.value("totalEnemies", 0);
        int stars = stats.value("stars", 0);
        float totalTime = stats.value("time", 0.0f);

        ImGuiIO& io = ImGui::GetIO();
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;

        // ── VICTORY Title ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.15f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##VictoryTitle", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(5.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
        ImGui::Text("VICTORY!");
        ImGui::PopStyleColor();
        ImGui::End();

        // ── Summary Panel ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.50f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(520, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.75f));
        ImGui::Begin("##Summary", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::SetWindowFontScale(1.8f);
        float titleW = ImGui::CalcTextSize("MISSION COMPLETE").x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleW) * 0.5f);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "MISSION COMPLETE");
        ImGui::Separator();
        ImGui::Spacing();

        char coinStr[64];
        sprintf(coinStr, "Gold Coins Collected: %d / %d", coins, tCoins);
        float coinW = (coinIcon ? 45.0f : 0.0f) + ImGui::CalcTextSize(coinStr).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - coinW) * 0.5f);

        // Coins summary
        if (coinIcon) {
            ImGui::Image((void*)(intptr_t)coinIcon->getOpenGLName(), ImVec2(40, 40));
            ImGui::SameLine();
        }
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", coinStr);

        ImGui::Spacing();

        char enemyStr[64];
        sprintf(enemyStr, "Guards Defeated: %d / %d", enemies, tEnemies);
        float enemyW = (enemyIcon ? 45.0f : 0.0f) + ImGui::CalcTextSize(enemyStr).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - enemyW) * 0.5f);

        // Enemies summary
        if (enemyIcon) {
            ImGui::Image((void*)(intptr_t)enemyIcon->getOpenGLName(), ImVec2(40, 40));
            ImGui::SameLine();
        }
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", enemyStr);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::SetWindowFontScale(1.4f);
        
        char timeStr[32];
        sprintf(timeStr, "Time: %d:%02d", (int)totalTime/60, (int)totalTime%60);
        float timeW = ImGui::CalcTextSize(timeStr).x;
        float starsW = ImGui::CalcTextSize("Stars: ***").x;
        float gap = 60.0f;
        float bottomRowW = timeW + gap + starsW;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - bottomRowW) * 0.5f);

        ImGui::Text("%s", timeStr);
        ImGui::SameLine(0, gap);
        ImGui::Text("Stars: ");
        ImGui::SameLine(0, 0);
        for(int i=0; i<3; ++i) {
            if (i < stars) ImGui::TextColored(ImVec4(1, 0.9f, 0, 1), "*");
            else ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1), "*");
            if (i < 2) ImGui::SameLine(0, 1);
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // ── Return Button ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.80f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(280, 0));
        ImGui::Begin("##VictoryBtn", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.8f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.55f, 0.1f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.35f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("  CONTINUE  ", ImVec2(260, 45))) {
            auto& appConfig = getApp()->getConfig();
            std::string nextPath = appConfig.value("next-level-config", "menu");

            if (nextPath == "menu") {
                getApp()->changeState("menu");
            } else {
                // Load the next level configuration
                std::ifstream f(nextPath);
                if (f) {
                    try {
                        nlohmann::json level = nlohmann::json::parse(f, nullptr, true, true);
                        if (level.contains("scene")) appConfig["scene"] = level["scene"];
                        if (level.contains("game")) appConfig["game"] = level["game"];
                        appConfig["play-level-config"] = nextPath;
                        getApp()->changeState("loading");
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to parse next level " << nextPath << ": " << e.what() << std::endl;
                        getApp()->changeState("menu");
                    }
                } else {
                    std::cerr << "Could not open next level file: " << nextPath << std::endl;
                    getApp()->changeState("menu");
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void onDestroy() override {
        if(coinIcon) delete coinIcon;
        if(enemyIcon) delete enemyIcon;
        delete rectangle;
        delete bgMaterial->texture;
        delete bgMaterial->shader;
        delete bgMaterial;
    }
};
