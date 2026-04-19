#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>

#include <functional>
#include <array>

// This struct is used to store the location and size of a button and the code it should execute when clicked
struct Button {
    // The position (of the top-left corner) of the button and its size in pixels
    glm::vec2 position, size;
    // The function that should be excuted when the button is clicked. It takes no arguments and returns nothing.
    std::function<void()> action;

    // This function returns true if the given vector v is inside the button. Otherwise, false is returned.
    // This is used to check if the mouse is hovering over the button.
    bool isInside(const glm::vec2& v) const {
        return position.x <= v.x && position.y <= v.y &&
            v.x <= position.x + size.x &&
            v.y <= position.y + size.y;
    }

    // This function returns the local to world matrix to transform a rectangle of size 1x1
    // (and whose top-left corner is at the origin) to be the button.
    glm::mat4 getLocalToWorld() const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) * 
            glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
    }
};

// This state shows how to use some of the abstractions we created to make a menu.
class Menustate: public our::State {

    // A meterial holding the menu shader and the menu texture to draw
    our::TexturedMaterial* menuMaterial;
    // A material to be used to highlight hovered buttons (we will use blending to create a negative effect).
    our::TintedMaterial * highlightMaterial;
    // A rectangle mesh on which the menu material will be drawn
    our::Mesh* rectangle;
    // A variable to record the time since the state is entered (it will be used for the fading effect).
    float time;
    // An array of the button that we can interact with
    std::array<Button, 2> buttons;
    // Selection marker icon
    our::Texture2D* markerIcon = nullptr;

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

        // Second, we create a material to highlight the hovered buttons
        highlightMaterial = new our::TintedMaterial();
        // Since the highlight is not textured, we used the tinted material shaders
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        // The tint is white since we will subtract the background color from it to create a negative effect.
        highlightMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        // To create a negative effect, we enable blending, set the equation to be subtract,
        // and set the factors to be one for both the source and the destination. 
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

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

        // Fill the positions, sizes and actions for the menu buttons
        // Note that we use lambda expressions to set the actions of the buttons.
        // A lambda expression consists of 3 parts:
        // - The capture list [] which is the variables that the lambda should remember because it will use them during execution.
        //      We store [this] in the capture list since we will use it in the action.
        // - The argument list () which is the arguments that the lambda should receive when it is called.
        //      We leave it empty since button actions receive no input.
        // - The body {} which contains the code to be executed. 
        buttons[0].position = {830.0f, 607.0f};
        buttons[0].size = {400.0f, 33.0f};
        buttons[0].action = [this](){this->getApp()->changeState("play");};

        buttons[1].position = {830.0f, 644.0f};
        buttons[1].size = {400.0f, 33.0f};
        buttons[1].action = [this](){this->getApp()->close();};

        // Load marker icon
        markerIcon = our::texture_utils::loadImage("assets/textures/coin_icon.png");
    }

    void onDraw(double deltaTime) override {
        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_SPACE)){
            // If the space key is pressed in this frame, go to the play state
            getApp()->changeState("play");
        } else if(keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            // If the escape key is pressed in this frame, exit the game
            getApp()->close();
        }

        // Get a reference to the mouse object and get the current mouse position
        auto& mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();

        // If the mouse left-button is just pressed, check if the mouse was inside
        // any menu button. If it was inside a menu button, run the action of the button.
        if(mouse.justPressed(0)){
            for(auto& button: buttons){
                if(button.isInside(mousePosition))
                    button.action();
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
            getApp()->changeState("play");
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

        // ── Controls hint ──
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.88f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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
        // Delete all the allocated resources
        if(markerIcon) delete markerIcon;
        delete rectangle;
        delete menuMaterial->texture;
        delete menuMaterial->shader;
        delete menuMaterial;
        delete highlightMaterial->shader;
        delete highlightMaterial;
    }
};