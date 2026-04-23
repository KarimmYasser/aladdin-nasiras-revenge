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
#include <json/json.hpp>

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

    void goToPlayLevel() {
        auto& cfg = getApp()->getConfig();
        if(cfg.contains("play-level-config") && cfg["play-level-config"].is_string()){
            std::string path = cfg["play-level-config"].get<std::string>();
            std::ifstream f(path);
            if(f){
                try {
                    nlohmann::json level = nlohmann::json::parse(f, nullptr, true, true);
                    if(level.contains("scene")) cfg["scene"] = level["scene"];
                    if(level.contains("game")) cfg["game"] = level["game"];
                } catch(const std::exception& e) {
                    std::cerr << "Failed to parse play-level-config " << path << ": " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Could not open play-level-config: " << path << std::endl;
            }
        }
        getApp()->changeState("play");
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

        // Load marker icon
        markerIcon = our::texture_utils::loadImage("assets/textures/coin_icon.png");

        // Start menu background music
        our::AudioSystem::instance().playMusic("assets/audio/menu.mp3");
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_SPACE)){
            goToPlayLevel();
        } else if(keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, exit the game
            getApp()->close();
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
        float btnW = 220.0f;
        float btnH = 50.0f;
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.55f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(btnW + 100, 0));
        ImGui::Begin("##MenuButtons", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowFontScale(2.0f);

        // Style buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
        if (ImGui::Button("   PLAY   ", ImVec2(btnW, btnH))) {
            goToPlayLevel();
        }
        if (ImGui::IsItemHovered() && markerIcon) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float coinY = min.y + (btnH - 32.0f) * 0.5f;
            ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)markerIcon->getOpenGLName(),
                ImVec2(max.x + 10.0f, coinY), ImVec2(max.x + 42.0f, coinY + 32.0f));
        }

        ImGui::Spacing();

        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
        if (ImGui::Button(" SETTINGS ", ImVec2(btnW, btnH))) {
            showSettings = !showSettings;
        }
        if (ImGui::IsItemHovered() && markerIcon) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float coinY = min.y + (btnH - 32.0f) * 0.5f;
            ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)markerIcon->getOpenGLName(),
                ImVec2(max.x + 10.0f, coinY), ImVec2(max.x + 42.0f, coinY + 32.0f));
        }

        ImGui::Spacing();

        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
        if (ImGui::Button("   EXIT   ", ImVec2(btnW, btnH))) {
            getApp()->close();
        }
        if (ImGui::IsItemHovered() && markerIcon) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            float coinY = min.y + (btnH - 32.0f) * 0.5f;
            ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)markerIcon->getOpenGLName(),
                ImVec2(max.x + 10.0f, coinY), ImVec2(max.x + 42.0f, coinY + 32.0f));
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::End();

        // ── Settings Panel ──
        if (showSettings) {
            float panelW = 360.0f;
            float panelH = 180.0f;
            ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.82f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(panelW, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.02f, 0.12f, 0.88f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.2f, 0.5f));
            ImGui::Begin("##SettingsPanel", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

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

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }

        // ── Controls hint ──
        float hintY = showSettings ? h * 0.96f : h * 0.88f;
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, hintY), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("##Hint", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.2f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::Text("Press SPACE to play  |  ESC to exit");
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
