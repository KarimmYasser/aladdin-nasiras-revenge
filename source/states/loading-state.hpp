#pragma once

#include <application.hpp>
#include <asset-loader.hpp>
#include <imgui.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <functional>

/**
 * @brief This state displays a loading screen and processes assets one-by-one.
 * 
 * By processing one asset per frame, we keep the UI responsive and the 
 * progress bar perfectly synced with the actual work being done.
 */
class LoadingState : public our::State {
    struct LoadTask {
        std::string type;
        std::string name;
        nlohmann::json desc;
    };

    std::vector<LoadTask> tasks;
    size_t currentTaskIndex = 0;
    float currentProgress = 0.0f;

    int cooldown = 10;

    void onInitialize() override {
        tasks.clear();
        currentTaskIndex = 0;
        currentProgress = 0.0f;
        cooldown = 15; // Give it a slight pause at 100% for smooth transition

        auto& config = getApp()->getConfig()["scene"];
        if (config.contains("assets")) {
            auto& assets = config["assets"];
            
            // Collect all individual assets into a single task list
            // We order them carefully: Shaders -> Textures -> Samplers -> Meshes -> Materials
            const std::vector<std::string> categories = {"shaders", "textures", "samplers", "meshes", "materials"};
            
            for (const auto& category : categories) {
                if (assets.contains(category) && assets[category].is_object()) {
                    for (auto& [name, desc] : assets[category].items()) {
                        tasks.push_back({category, name, desc});
                    }
                }
            }
        }
    }

    void onDraw(double deltaTime) override {
        // Clear screen to a dark purple/night theme matching the game's aesthetic
        glClearColor(0.04f, 0.02f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (tasks.empty()) {
            getApp()->changeState("play");
            return;
        }

        // Process ONE task per frame
        if (currentTaskIndex < tasks.size()) {
            const auto& task = tasks[currentTaskIndex];
            
            if (task.type == "shaders") our::AssetLoader<our::ShaderProgram>::deserializeSingle(task.name, task.desc);
            else if (task.type == "textures") our::AssetLoader<our::Texture2D>::deserializeSingle(task.name, task.desc);
            else if (task.type == "samplers") our::AssetLoader<our::Sampler>::deserializeSingle(task.name, task.desc);
            else if (task.type == "meshes") our::AssetLoader<our::Mesh>::deserializeSingle(task.name, task.desc);
            else if (task.type == "materials") our::AssetLoader<our::Material>::deserializeSingle(task.name, task.desc);
            
            currentTaskIndex++;
        } else {
            // Once all tasks are done, wait a few frames for visual polish then switch
            static int cooldown = 10;
            if (cooldown-- <= 0) {
                getApp()->changeState("play");
                cooldown = 10; // reset for next time
            }
        }

        // Calculate target progress based on actual task completion
        float targetProgress = tasks.empty() ? 1.0f : (float)currentTaskIndex / (float)tasks.size();
        
        // Use very fast interpolation to keep the bar feeling responsive but smooth
        currentProgress = glm::mix(currentProgress, targetProgress, (float)deltaTime * 10.0f);
        if (std::abs(currentProgress - targetProgress) < 0.001f) currentProgress = targetProgress;
    }

    void onImmediateGui() override {
        ImGuiIO& io = ImGui::GetIO();
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;

        // Center the Loading text
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##LoadingScreen", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        
        ImGui::SetWindowFontScale(3.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        ImGui::Text("ENTERING AGRABAH");
        ImGui::PopStyleColor();

        ImGui::SetWindowFontScale(1.5f);
        ImGui::Spacing();
        
        // Simple animated dots for visual activity
        float t = (float)glfwGetTime();
        int dotCount = (int)(t * 3.0f) % 4;
        std::string dots = "Loading";
        for(int i = 0; i < dotCount; ++i) dots += ".";
        ImGui::Text("%s", dots.c_str());

        ImGui::End();
        
        // Progress bar at the bottom
        float barWidth = w * 0.6f;
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.65f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##ProgressBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));
        ImGui::ProgressBar(currentProgress, ImVec2(barWidth, 12.0f), "");
        ImGui::PopStyleColor();
        
        ImGui::End();
    }

    void onDestroy() override {
        // Assets persist into PlayState
    }
};
