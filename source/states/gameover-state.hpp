#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <iomanip>

// Game Over State — displayed when the player loses all health/lives.
// Shows a themed background with fade-in effect.
// Press SPACE/ENTER to return to menu, ESC to quit.
class GameOverState : public our::State {

    our::TexturedMaterial* bgMaterial;
    our::Mesh* rectangle;
    float time;

    // UI Icons
    our::Texture2D* enemyIcon = nullptr;
    our::Texture2D* heartIcon = nullptr;

    void onInitialize() override {
        // Create the background material
        bgMaterial = new our::TexturedMaterial();
        bgMaterial->shader = new our::ShaderProgram();
        bgMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        bgMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        bgMaterial->shader->link();
        bgMaterial->texture = our::texture_utils::loadImage("assets/textures/gameover_bg.png");
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
        enemyIcon = our::texture_utils::loadImage("assets/textures/monkey.png");
        heartIcon = our::texture_utils::loadImage("assets/textures/heart_icon.png");
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

        // Setup orthographic projection (same technique as menu-state)
        glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        // Fade-in effect over 1.5 seconds
        time += (float)deltaTime;
        bgMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 1.5f, time));

        // Draw fullscreen background
        bgMaterial->setup();
        bgMaterial->shader->set("transform", VP * M);
        rectangle->draw();
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;

        // ── GAME OVER Title ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.20f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##GameOverTitle", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(4.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
        ImGui::Text("GAME OVER");
        ImGui::PopStyleColor();
        ImGui::End();

        // ── Summary Panel ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.50f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0, 0, 0.8f));
        ImGui::Begin("##GameOverSummary", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::SetWindowFontScale(1.6f);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("DEFEATED").x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "DEFEATED");
        ImGui::Separator();
        ImGui::Spacing();

        if (enemyIcon) {
            ImGui::Image((void*)(intptr_t)enemyIcon->getOpenGLName(), ImVec2(40, 40));
            ImGui::SameLine();
        }
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Nasira's forces were too strong...");

        ImGui::Spacing();

        if (heartIcon) {
            ImGui::Image((void*)(intptr_t)heartIcon->getOpenGLName(), ImVec2(40, 40), ImVec2(0,0), ImVec2(1,1), ImVec4(0.5, 0.5, 0.5, 0.5));
            ImGui::SameLine();
        }
        ImGui::AlignTextToFramePadding();
        ImGui::Text("All lives lost.");

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // ── Return Button ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.80f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(280, 0));
        ImGui::Begin("##GameOverBtn", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.8f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (ImGui::Button("  RETRY  ", ImVec2(260, 45))) {
            getApp()->changeState("menu");
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void onDestroy() override {
        if(enemyIcon) delete enemyIcon;
        if(heartIcon) delete heartIcon;
        delete rectangle;
        delete bgMaterial->texture;
        delete bgMaterial->shader;
        delete bgMaterial;
    }
};
