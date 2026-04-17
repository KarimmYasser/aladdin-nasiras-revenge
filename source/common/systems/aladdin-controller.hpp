#pragma once

#include "../ecs/world.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/camera.hpp"
#include "../components/enemy.hpp"
#include "../application.hpp"
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace our {

    /**
     * @brief System that handles character movement, jumping, and states for Aladdin.
     * 
     * It reads keyboard input to update the entity's position, applies "fake" gravity
     * until a physics system is available, and manages character rotation.
     */
    class AladdinControllerSystem {
        Application* app; // Pointer to the application for input handling

    public:
        // Initialize with the application pointer
        void enter(Application* app) {
            this->app = app;
        }

        /**
         * @brief Updates every entity in the world containing a AladdinControllerComponent.
         * 
         * @param world The scene world containing entities.
         * @param deltaTime The time elapsed since the last frame.
         */
        void update(World* world, float deltaTime) {
            for(auto entity : world->getEntities()){
                AladdinControllerComponent* aladdin = entity->getComponent<AladdinControllerComponent>();
                if(!aladdin) continue;

                auto& keyboard = app->getKeyboard();

                // 1. Get movement direction from keyboard input
                glm::vec3 moveDir = {0, 0, 0};
                if(keyboard.isPressed(GLFW_KEY_W)) moveDir.z -= 1.0f; // Forward
                if(keyboard.isPressed(GLFW_KEY_S)) moveDir.z += 1.0f; // Backward
                if(keyboard.isPressed(GLFW_KEY_A)) moveDir.x -= 1.0f; // Left
                if(keyboard.isPressed(GLFW_KEY_D)) moveDir.x += 1.0f; // Right

                // 2. Apply movement (horizontal)
                // TODO (Physics): Replace manual position updates with force/velocity
                // application on the player's Rigidbody/Collider.
                if(glm::length(moveDir) > 0.001f) {
                    moveDir = glm::normalize(moveDir);
                    entity->localTransform.position += moveDir * aladdin->speed * deltaTime;

                    // Rotate entity to face the movement direction
                    float targetYaw = glm::atan(moveDir.x, moveDir.z);
                    // Smoothly interpolate rotation
                    float currentYaw = entity->localTransform.rotation.y;
                    float diff = targetYaw - currentYaw;
                    while(diff > glm::pi<float>()) diff -= 2 * glm::pi<float>();
                    while(diff < -glm::pi<float>()) diff += 2 * glm::pi<float>();
                    entity->localTransform.rotation.y += diff * aladdin->rotationSpeed * deltaTime;
                }

                // 3. Handle Jumping (Vertical logic)
                // TODO (Physics): Use the physics engine's gravity instead of this constant.
                // Mock Gravity (Temporary until Physics Dev finishes PhysicsSystem)
                const float gravity = -20.0f; 
                aladdin->velocity.y += gravity * deltaTime;

                // Apply vertical velocity
                entity->localTransform.position.y += aladdin->velocity.y * deltaTime;

                // TODO (Physics): Replace this Y=0 check with real collision detection
                // from the PhysicsSystem (isGrounded should come from the Collider).
                // Simple floor check (Mocking ground at Y=0)
                if(entity->localTransform.position.y <= 0.0f) {
                    entity->localTransform.position.y = 0.0f;
                    aladdin->velocity.y = 0.0f;
                    aladdin->isGrounded = true;
                } else {
                    aladdin->isGrounded = false;
                }

                // Trigger Jump
                if(keyboard.justPressed(GLFW_KEY_SPACE) && aladdin->isGrounded) {
                    aladdin->velocity.y = aladdin->jumpForce;
                    aladdin->isGrounded = false;
                }

                // 3.5. Update Invincibility Timer
                if(aladdin->invincibilityTimer > 0.0f) {
                    aladdin->invincibilityTimer -= deltaTime;
                }

                // 4. Handle Melee Attack (Sword)
                if(keyboard.justPressed(GLFW_KEY_F) && !aladdin->isAttacking) {
                    aladdin->isAttacking = true;
                    aladdin->attackTimer = 0.4f; // Attack for 0.4 seconds
                    // TODO (Graphics): Trigger sword "swoosh" sound (Member 1)
                    // TODO (Animation): Trigger the "Sword Slash" animation (Member 1)
                }

                if(aladdin->isAttacking) {
                    aladdin->attackTimer -= deltaTime;
                    if(aladdin->attackTimer <= 0.0f) {
                        aladdin->isAttacking = false;
                    }
                    // Visual effect placeholder: slightly shake or tilt model
                    
                    // Simple Combat Test: Check for nearby "Target" or "Enemy" entities
                    // TODO (Physics): Replace this distance-based check with the PhysicsSystem's
                    // collision detection between the sword's hitbox collider and enemy colliders.
                    // Damage should only be applied at a specific frame of the animation.
                    for(auto other : world->getEntities()){
                        if(other == entity) continue;

                        // Check for Target objects (from original mock)
                        if(other->name.find("Target") != std::string::npos){
                            float dist = glm::distance(entity->localTransform.position, other->localTransform.position);
                            if(dist < 2.0f){
                                // "Hit" the target - for now just log and maybe move it
                                std::cout << "Sword Hit: " << other->name << "!" << std::endl;
                                // Shift it slightly to show impact
                                glm::vec3 dir = glm::normalize(other->localTransform.position - entity->localTransform.position);
                                other->localTransform.position += dir * 0.1f;
                            }
                        }

                        // Check for Real Enemies
                        EnemyComponent* enemy = other->getComponent<EnemyComponent>();
                        if(enemy && enemy->currentState != EnemyComponent::State::DEAD) {
                            float dist = glm::distance(entity->localTransform.position, other->localTransform.position);
                            if(dist < 2.5f) { // Slightly larger range for Aladdin's sword
                                enemy->health -= 25; // Aladdin deals 25 damage per hit
                                std::cout << "[AladdinSystem] Hit " << other->name << "! Enemy Health: " << enemy->health << std::endl;
                                
                                if(enemy->health <= 0) {
                                    enemy->currentState = EnemyComponent::State::DEAD;
                                    std::cout << "[AladdinSystem] " << other->name << " defeated!" << std::endl;
                                }
                                // To prevent hitting multiple times in one frame, we could break or add a hit cooldown
                                // But since this is a simple system, we'll just allow it for now.
                            }
                        }
                    }
                }

                // 5. Handle Ranged Attack (Apple Throw)
                if(keyboard.justPressed(GLFW_KEY_R) && !aladdin->isThrowing && aladdin->appleCount > 0) {
                    aladdin->isThrowing = true;
                    aladdin->throwTimer = 0.3f; // Throw animation for 0.3 seconds
                    aladdin->appleCount--;
                    // TODO (Animation): Trigger the "Throw" animation (Member 1)
                    // TODO (Gameplay): Implement an actual Projectile System (Member 3 - future phase).
                    // This should spawn a new entity with an Apple mesh, Collider, and Velocity.
                }

                if(aladdin->isThrowing) {
                    aladdin->throwTimer -= deltaTime;
                    if(aladdin->throwTimer <= 0.0f) {
                        aladdin->isThrowing = false;
                    }
                    
                    // Simple Throw Test: Log when an apple is "thrown" towards a target
                    // TODO (Physics/Gameplay): In the real system, the spawned projectile
                    // will handle its own collision with enemies independently.
                }

                // 6. Handle Camera Follow
                if(aladdin->enableCameraFollow){
                    // Search for a camera entity
                    Entity* cameraEntity = nullptr;
                    for(auto e : world->getEntities()){
                        if(e->getComponent<CameraComponent>()){
                            cameraEntity = e;
                            break;
                        }
                    }

                    if(cameraEntity){
                        // The target position is the player's position + the offset
                        glm::vec3 targetPosition = entity->localTransform.position + aladdin->cameraOffset;
                        
                        // Apply smoothing (Simple linear interpolation/lerp)
                        if(aladdin->cameraSmoothing > 0.0f){
                            // factor = 1 - e^(-smoothing * dt) is a common way to do framerate-independent smoothing
                            float factor = 1.0f - glm::exp(-aladdin->cameraSmoothing * deltaTime);
                            cameraEntity->localTransform.position = glm::mix(cameraEntity->localTransform.position, targetPosition, factor);
                        } else {
                            // Instant follow
                            cameraEntity->localTransform.position = targetPosition;
                        }

                        // Optional: Make camera look at Aladdin
                        // The game description mentions a free-roaming camera in open 3D spaces.
                        // We might need to add logic to adjust the camera's orientation here.
                    }
                }
            }
        }

        // For Debugging only
        // TODO - Remove or disable in production builds
        /**
         * @brief Renders the ImGui debug window for Aladdin.
         * 
         * @param world The scene world containing entities.
         */
        void onImmediateGui(World* world) {
            // Find the Aladdin controller component
            AladdinControllerComponent* aladdin = nullptr;
            for(auto entity : world->getEntities()){
                aladdin = entity->getComponent<AladdinControllerComponent>();
                if(aladdin) break;
            }

            // If no Aladdin component found, nothing to do
            if(!aladdin) return;

            // Display a HUD-like window for Aladdin status
            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300.0f, 180.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Aladdin HUD", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            
            // Visual Health Bar (Green to Red)
            float healthFraction = (float)aladdin->health / 100.0f;
            ImVec4 healthColor = ImVec4(1.0f - healthFraction, healthFraction, 0.0f, 1.0f);
            ImGui::Text("Health: %d / 100", aladdin->health);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
            ImGui::ProgressBar(healthFraction, ImVec2(-1.0f, 20.0f), "");
            ImGui::PopStyleColor();

            // Visual Lives Display
            ImGui::Spacing();
            ImGui::Text("Lives Remaining:");
            for(int i = 0; i < aladdin->lives; ++i) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "<3"); // Red hearts for lives
            }
            if(aladdin->lives <= 0) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "NONE - GAME OVER");
            }

            ImGui::Separator();
            ImGui::Text("Inventory:");
            ImGui::Text("Coins: %d | Gems: %d | Apples: %d", aladdin->coinCount, aladdin->gemCount, aladdin->appleCount);
            
            if(aladdin->invincibilityTimer > 0.0f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "INVINCIBLE: %.1fs", aladdin->invincibilityTimer);
            }

            ImGui::End();

            // Original Debug Window
            ImGui::Begin("Aladdin Controller Debug");
            
            ImGui::Text("State Info:");
            ImGui::Value("Health", aladdin->health);
            ImGui::Value("Coins", aladdin->coinCount);
            ImGui::Value("Gems", aladdin->gemCount);
            ImGui::Value("Apples", aladdin->appleCount);
            ImGui::Separator();
            ImGui::Value("Is Grounded", aladdin->isGrounded);
            
            ImGui::Separator();
            ImGui::Text("Combat State:");
            if (aladdin->isAttacking) {
                ImGui::ProgressBar(aladdin->attackTimer / 0.4f, ImVec2(0.0f, 0.0f), "Sword Attack");
            } else {
                ImGui::Text("Sword: Ready (Press F)");
            }

            if (aladdin->isThrowing) {
                ImGui::ProgressBar(aladdin->throwTimer / 0.3f, ImVec2(0.0f, 0.0f), "Throwing Apple");
            } else {
                ImGui::Text("Apple: Ready (Press R) [%d left]", aladdin->appleCount);
            }
            
            ImGui::Separator();
            ImGui::Text("Movement Config:");
            ImGui::DragFloat("Speed", &aladdin->speed, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Jump Force", &aladdin->jumpForce, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Rotation Speed", &aladdin->rotationSpeed, 0.1f, 0.0f, 20.0f);

            ImGui::Separator();
            ImGui::Text("Camera Follow:");
            ImGui::Checkbox("Enable Follow", &aladdin->enableCameraFollow);
            ImGui::DragFloat3("Camera Offset", &aladdin->cameraOffset[0], 0.1f);
            ImGui::DragFloat("Camera Smoothing", &aladdin->cameraSmoothing, 0.1f, 0.0f, 20.0f);

            ImGui::Separator();
            ImGui::Text("Physics State:");
            ImGui::DragFloat3("Velocity", &aladdin->velocity[0], 0.1f);
            
            // Allow manual teleport/reset for testing
            if(ImGui::Button("Reset Position")) {
                aladdin->getOwner()->localTransform.position = {0, 0, 0};
                aladdin->velocity = {0, 0, 0};
            }

            ImGui::End();
        }
    };
}
