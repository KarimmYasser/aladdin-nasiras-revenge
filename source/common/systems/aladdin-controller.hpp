#pragma once

#include "../ecs/world.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/camera.hpp"
#include "../components/enemy.hpp"
#include "../components/breakable.hpp"
#include "../components/collectible.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/movement.hpp"
#include "../components/rigid-body.hpp"
#include "../asset-loader.hpp"
#include "../application.hpp"
#include "../input/mouse.hpp"
#include <imgui.h>
#include <cmath>

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

        /// Unlock cursor if follow-cam mouse look had it captured (call from Playstate onDestroy / scene change).
        void unlockFollowCameraMouse(World* world) {
            if(!app || !world) return;
            GLFWwindow* win = app->getWindow();
            for(auto entity : world->getEntities()){
                if(auto* a = entity->getComponent<AladdinControllerComponent>()) {
                    if(a->followCamMouseLocked) {
                        Mouse::unlockMouse(win);
                        a->followCamMouseLocked = false;
                        a->followCamSkipNextLookDelta = false;
                    }
                }
            }
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

                if(aladdin->lives <= 0) continue;

                auto& keyboard = app->getKeyboard();

                // 1. Get movement direction from keyboard input
                glm::vec3 moveDir = {0, 0, 0};
                if(keyboard.isPressed(GLFW_KEY_W)) moveDir.z -= 1.0f; // Forward
                if(keyboard.isPressed(GLFW_KEY_S)) moveDir.z += 1.0f; // Backward
                if(keyboard.isPressed(GLFW_KEY_A)) moveDir.x -= 1.0f; // Left
                if(keyboard.isPressed(GLFW_KEY_D)) moveDir.x += 1.0f; // Right

                MovementComponent* movement = entity->getComponent<MovementComponent>();
                const bool useMovementSystem = movement != nullptr;

                const float gravity = -20.0f;
                aladdin->velocity.y += gravity * deltaTime;

                if(useMovementSystem) {
                    glm::vec3 horizVel(0.0f);
                    if(glm::length(moveDir) > 0.001f) {
                        moveDir = glm::normalize(moveDir);
                        // Local WASD in character space (W = model -Z) rotated into world by facing yaw
                        const float yf = aladdin->facingYaw;
                        const float c = glm::cos(yf);
                        const float s = glm::sin(yf);
                        const glm::vec3 worldDir(
                            moveDir.x * c + moveDir.z * s,
                            0.0f,
                            -moveDir.x * s + moveDir.z * c
                        );
                        horizVel = worldDir * aladdin->speed;
                        // Pure strafe (A/D only): move sideways without spinning facing or the follow camera.
                        if(std::abs(moveDir.z) > 0.001f) {
                            const float targetYaw = std::atan2(worldDir.x, worldDir.z);
                            float currentYaw = aladdin->facingYaw;
                            float diff = targetYaw - currentYaw;
                            while(diff > glm::pi<float>()) diff -= 2.0f * glm::pi<float>();
                            while(diff < -glm::pi<float>()) diff += 2.0f * glm::pi<float>();
                            aladdin->facingYaw = currentYaw + diff * aladdin->rotationSpeed * deltaTime;
                        }
                        entity->localTransform.rotation.x = 0.0f;
                        entity->localTransform.rotation.y = aladdin->facingYaw + AladdinControllerComponent::meshYawVisualOffset;
                        entity->localTransform.rotation.z = 0.0f;
                    }
                    movement->linearVelocity = glm::vec3(horizVel.x, aladdin->velocity.y, horizVel.z);
                    if(keyboard.justPressed(GLFW_KEY_SPACE) && aladdin->isGrounded) {
                        aladdin->velocity.y = aladdin->jumpForce;
                        aladdin->isGrounded = false;
                        movement->linearVelocity.y = aladdin->velocity.y;
                    }
                } else {
                    if(glm::length(moveDir) > 0.001f) {
                        moveDir = glm::normalize(moveDir);
                        const float yf = aladdin->facingYaw;
                        const float c = glm::cos(yf);
                        const float s = glm::sin(yf);
                        const glm::vec3 worldDir(
                            moveDir.x * c + moveDir.z * s,
                            0.0f,
                            -moveDir.x * s + moveDir.z * c
                        );
                        entity->localTransform.position += worldDir * aladdin->speed * deltaTime;
                        if(std::abs(moveDir.z) > 0.001f) {
                            const float targetYaw = std::atan2(worldDir.x, worldDir.z);
                            float currentYaw = aladdin->facingYaw;
                            float diff = targetYaw - currentYaw;
                            while(diff > glm::pi<float>()) diff -= 2.0f * glm::pi<float>();
                            while(diff < -glm::pi<float>()) diff += 2.0f * glm::pi<float>();
                            aladdin->facingYaw = currentYaw + diff * aladdin->rotationSpeed * deltaTime;
                        }
                        entity->localTransform.rotation.x = 0.0f;
                        entity->localTransform.rotation.y = aladdin->facingYaw + AladdinControllerComponent::meshYawVisualOffset;
                        entity->localTransform.rotation.z = 0.0f;
                    }
                    entity->localTransform.position.y += aladdin->velocity.y * deltaTime;
                    if(entity->localTransform.position.y <= 0.0f) {
                        entity->localTransform.position.y = 0.0f;
                        aladdin->velocity.y = 0.0f;
                        aladdin->isGrounded = true;
                    } else {
                        aladdin->isGrounded = false;
                    }
                    if(keyboard.justPressed(GLFW_KEY_SPACE) && aladdin->isGrounded) {
                        aladdin->velocity.y = aladdin->jumpForce;
                        aladdin->isGrounded = false;
                    }
                }

                // 3.5. Update Invincibility Timer
                if(aladdin->invincibilityTimer > 0.0f) {
                    aladdin->invincibilityTimer -= deltaTime;
                }

                // 4. Handle Melee Attack (Sword)
                if(keyboard.justPressed(GLFW_KEY_F) && !aladdin->isAttacking) {
                    aladdin->isAttacking = true;
                    aladdin->attackTimer = 0.4f; // Attack for 0.4 seconds
                    aladdin->hitEntities.clear(); // Clear the list of entities hit in the previous attack
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
                            // Check if this enemy was already hit during the current attack
                            bool alreadyHit = false;
                            for(auto e : aladdin->hitEntities) {
                                if(e == other) {
                                    alreadyHit = true;
                                    break;
                                }
                            }
                            if(alreadyHit) continue;

                            // TODO (Member 2): Replace this distance-based check with the PhysicsSystem's
                            // collision detection once the ColliderComponent is ready.
                            float dist = glm::distance(entity->localTransform.position, other->localTransform.position);
                            if(dist < 2.5f) { // Slightly larger range for Aladdin's sword
                                enemy->health -= 25; // Aladdin deals 25 damage per hit
                                aladdin->hitEntities.push_back(other); // Mark this enemy as hit
                                std::cout << "[AladdinSystem] Hit " << other->name << "! Enemy Health: " << enemy->health << std::endl;
                                
                                if(enemy->health <= 0) {
                                    enemy->currentState = EnemyComponent::State::DEAD;
                                    aladdin->enemyCount += 1;
                                    std::cout << "[AladdinSystem] " << other->name << " defeated!" << std::endl;
                                }
                                // To prevent hitting multiple times in one frame, we could break or add a hit cooldown
                                // But since this is a simple system, we'll just allow it for now.
                            }
                        }

                        // Check for Breakable Props (Pots)
                        BreakableComponent* breakable = other->getComponent<BreakableComponent>();
                        if(breakable) {
                            // Check if this breakable was already hit during the current attack
                            bool alreadyHit = false;
                            for(auto e : aladdin->hitEntities) {
                                if(e == other) {
                                    alreadyHit = true;
                                    break;
                                }
                            }
                            if(alreadyHit) continue;

                            // TODO (Member 2): Replace this distance-based check with the PhysicsSystem's
                            // collision detection once the ColliderComponent is ready.
                            float dist = glm::distance(entity->localTransform.position, other->localTransform.position);
                            if(dist < 2.0f) {
                                aladdin->hitEntities.push_back(other); // Mark this breakable as hit
                                std::cout << "[AladdinSystem] Broke " << other->name << "!" << std::endl;
                                
                                // Spawn multiple loot items if defined
                                for(size_t i = 0; i < breakable->lootItems.size(); ++i) {
                                    const auto& lootEntry = breakable->lootItems[i];
                                    
                                    Entity* loot = world->add();
                                    loot->name = "Dropped_" + lootEntry.type + "_" + std::to_string(i);
                                    
                                    // Scatter logic: Use sine and cosine to distribute items in a wider circle around the pot
                                    float angle = ((float)i / (float)breakable->lootItems.size()) * 2.0f * glm::pi<float>();
                                    float radius = 3.5f; // Increased distance from the center for more scattering
                                    glm::vec3 scatterOffset = glm::vec3(glm::cos(angle) * radius, 0.7f, glm::sin(angle) * radius);
                                    
                                    loot->localTransform.position = other->localTransform.position + scatterOffset;

                                    Mesh* lootMesh = nullptr;
                                    Material* lootMat = nullptr;
                                    glm::vec3 lootScale(1.0f);
                                    float lootRotSpeed = 2.0f;
                                    float lootBob = 0.15f;
                                    if(lootEntry.type == "coin") {
                                        lootMesh = AssetLoader<Mesh>::get("coin_mesh");
                                        lootMat = AssetLoader<Material>::get("coin-mat");
                                        lootScale = glm::vec3(4.6f);
                                        lootRotSpeed = 3.0f;
                                        lootBob = 0.15f;
                                    } else if(lootEntry.type == "apple") {
                                        lootMesh = AssetLoader<Mesh>::get("apple_mesh");
                                        lootMat = AssetLoader<Material>::get("lit-apple");
                                        lootScale = glm::vec3(1.3f);
                                        lootRotSpeed = 2.0f;
                                        lootBob = 0.12f;
                                    } else {
                                        lootMesh = AssetLoader<Mesh>::get("coin_mesh");
                                        lootMat = AssetLoader<Material>::get("coin-mat");
                                        lootScale = glm::vec3(4.6f);
                                    }
                                    loot->localTransform.scale = lootScale;

                                    auto mr = loot->addComponent<MeshRendererComponent>();
                                    mr->mesh = lootMesh ? lootMesh : AssetLoader<Mesh>::get("cube");
                                    mr->material = lootMat ? lootMat : AssetLoader<Material>::get("coin-mat");

                                    auto coll = loot->addComponent<CollectibleComponent>();
                                    if(lootEntry.type == "coin") coll->type = CollectibleComponent::Type::COIN;
                                    else if(lootEntry.type == "gem") coll->type = CollectibleComponent::Type::GEM;
                                    else if(lootEntry.type == "apple") coll->type = CollectibleComponent::Type::APPLE;
                                    coll->value = lootEntry.value;
                                    coll->rotationSpeed = lootRotSpeed;
                                    coll->bobbingHeight = lootBob;
                                }

                                // Mark the pot for removal
                                world->markForRemoval(other);
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

            }
        }

        /**
         * @brief After MovementSystem integration: snap player to floor and update follow camera.
         */
        void lateUpdate(World* world, float deltaTime) {
            for(auto entity : world->getEntities()){
                AladdinControllerComponent* aladdin = entity->getComponent<AladdinControllerComponent>();
                if(!aladdin || aladdin->lives <= 0) continue;

                MovementComponent* movement = entity->getComponent<MovementComponent>();
                auto* rb = entity->getComponent<RigidBodyComponent>();
                const bool physicsMovesBody = rb && rb->type == RigidBodyType::Dynamic;
                if(movement && !physicsMovesBody) {
                    if(entity->localTransform.position.y <= 0.0f) {
                        entity->localTransform.position.y = 0.0f;
                        aladdin->velocity.y = 0.0f;
                        movement->linearVelocity.y = 0.0f;
                        aladdin->isGrounded = true;
                    } else {
                        aladdin->isGrounded = false;
                    }
                }

            }
        }

        /**
         * @brief Positions the gameplay camera from Aladdin's transform (call after physics so dynamic bodies match visuals).
         */
        void updateFollowCamera(World* world, float deltaTime) {
            if(!world) return;

            for(auto entity : world->getEntities()){
                AladdinControllerComponent* aladdin = entity->getComponent<AladdinControllerComponent>();
                if(!aladdin || aladdin->lives <= 0 || !aladdin->enableCameraFollow) continue;

                Entity* cameraEntity = nullptr;
                for(auto e : world->getEntities()) {
                    if(e->getComponent<CameraComponent>()) {
                        cameraEntity = e;
                        break;
                    }
                }
                if(!cameraEntity) continue;

                if(app && app->getMouse().isEnabled()) {
                    auto& mouse = app->getMouse();
                    const int btn = aladdin->mouseLookButton;
                    GLFWwindow* win = app->getWindow();
                    if(mouse.isPressed(btn)) {
                        if(!aladdin->followCamMouseLocked) {
                            Mouse::lockMouse(win);
                            aladdin->followCamMouseLocked = true;
                            aladdin->followCamSkipNextLookDelta = true;
                        }
                        glm::vec2 d = mouse.getMouseDelta();
                        if(aladdin->followCamSkipNextLookDelta) {
                            aladdin->followCamSkipNextLookDelta = false;
                            d = glm::vec2(0.0f);
                        }
                        aladdin->cameraYawOffset += d.x * aladdin->mouseLookSensitivity;
                        aladdin->cameraPitchOffset += d.y * aladdin->mouseLookSensitivity;
                        while(aladdin->cameraYawOffset > glm::pi<float>()) aladdin->cameraYawOffset -= 2.0f * glm::pi<float>();
                        while(aladdin->cameraYawOffset < -glm::pi<float>()) aladdin->cameraYawOffset += 2.0f * glm::pi<float>();
                        const float basePitch = (aladdin->cameraMode == AladdinCameraMode::FirstPerson)
                            ? aladdin->firstPersonPitch : aladdin->thirdPersonPitch;
                        float totalPitch = basePitch + aladdin->cameraPitchOffset;
                        totalPitch = glm::clamp(totalPitch, glm::radians(-80.0f), glm::radians(18.0f));
                        aladdin->cameraPitchOffset = totalPitch - basePitch;
                    } else if(aladdin->followCamMouseLocked) {
                        Mouse::unlockMouse(win);
                        aladdin->followCamMouseLocked = false;
                        aladdin->followCamSkipNextLookDelta = false;
                    }
                } else if(aladdin->followCamMouseLocked && app) {
                    Mouse::unlockMouse(app->getWindow());
                    aladdin->followCamMouseLocked = false;
                    aladdin->followCamSkipNextLookDelta = false;
                }

                const glm::vec3& pos = entity->localTransform.position;
                const float yaw = aladdin->facingYaw + aladdin->cameraYawOffset;
                // Match Transform::toMat4 + view forward: local -Z becomes this world direction on XZ
                glm::vec3 forward(-glm::sin(yaw), 0.0f, -glm::cos(yaw));
                if(glm::dot(forward, forward) > 1e-8f) forward = glm::normalize(forward);
                glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
                const glm::vec3 up(0.0f, 1.0f, 0.0f);

                glm::vec3 targetPosition;
                glm::vec3 targetRotationEuler;

                if(aladdin->cameraMode == AladdinCameraMode::ThirdPerson) {
                    const glm::vec3& o = aladdin->cameraOffset;
                    targetPosition = pos - forward * o.z + right * o.x + up * o.y;
                    targetRotationEuler = {aladdin->thirdPersonPitch + aladdin->cameraPitchOffset, yaw, 0.0f};
                } else {
                    targetPosition = pos + right * aladdin->firstPersonCameraOffset.x + up * aladdin->firstPersonCameraOffset.y
                        + forward * aladdin->firstPersonCameraOffset.z;
                    targetRotationEuler = {aladdin->firstPersonPitch + aladdin->cameraPitchOffset, yaw, 0.0f};
                }

                if(aladdin->cameraSmoothing > 0.0f) {
                    // Clamp dt so a hitch (e.g. console I/O) does not apply a huge blend in one frame.
                    const float camDt = glm::min(deltaTime, 0.05f);
                    const float factor = 1.0f - glm::exp(-aladdin->cameraSmoothing * camDt);
                    cameraEntity->localTransform.position = glm::mix(cameraEntity->localTransform.position, targetPosition, factor);
                    glm::vec3& r = cameraEntity->localTransform.rotation;
                    const float yawCur = r.y;
                    const float yawTgt = targetRotationEuler.y;
                    float dy = yawTgt - yawCur;
                    while(dy > glm::pi<float>()) dy -= 2.0f * glm::pi<float>();
                    while(dy < -glm::pi<float>()) dy += 2.0f * glm::pi<float>();
                    // Snap pitch/roll: initial scene camera pitch (-32° in level) was being slowly lerped toward
                    // third-person pitch, so any frame spike looked like a sudden "twist". Yaw stays smoothed.
                    r.x = targetRotationEuler.x;
                    r.y = yawCur + dy * factor;
                    r.z = targetRotationEuler.z;
                } else {
                    cameraEntity->localTransform.position = targetPosition;
                    cameraEntity->localTransform.rotation = targetRotationEuler;
                }

                entity->localTransform.rotation.x = 0.0f;
                entity->localTransform.rotation.y = aladdin->facingYaw + AladdinControllerComponent::meshYawVisualOffset;
                entity->localTransform.rotation.z = 0.0f;

                if(auto* meshRenderer = entity->getComponent<MeshRendererComponent>()) {
                    meshRenderer->visible = (aladdin->cameraMode != AladdinCameraMode::FirstPerson);
                }

                if(auto* rb = entity->getComponent<RigidBodyComponent>(); rb && rb->type == RigidBodyType::Dynamic) {
                    aladdin->isGrounded = std::abs(rb->velocity.y) < 0.25f;
                    aladdin->velocity.y = rb->velocity.y;
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
            
            if(aladdin->hasKey) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " | [KEY]");
            }
            
            if(aladdin->invincibilityTimer > 0.0f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "INVINCIBLE: %.1fs", aladdin->invincibilityTimer);
            }

            ImGui::Separator();
            ImGui::Text("Respawn Position:");
            ImGui::Text("%.1f, %.1f, %.1f", aladdin->respawnPosition.x, aladdin->respawnPosition.y, aladdin->respawnPosition.z);

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
            int mode = (aladdin->cameraMode == AladdinCameraMode::FirstPerson) ? 1 : 0;
            if(ImGui::Combo("Camera mode", &mode, "Third person\0First person\0")){
                aladdin->cameraMode = mode ? AladdinCameraMode::FirstPerson : AladdinCameraMode::ThirdPerson;
            }
            ImGui::DragFloat3("Third offset (x=side,y=up,z=back)", &aladdin->cameraOffset[0], 0.1f);
            ImGui::DragFloat3("First-person offset", &aladdin->firstPersonCameraOffset[0], 0.05f);
            ImGui::DragFloat("First-person pitch (rad)", &aladdin->firstPersonPitch, 0.01f, -1.2f, 1.2f);
            ImGui::DragFloat("Third-person pitch (rad)", &aladdin->thirdPersonPitch, 0.01f, -0.8f, 0.3f);
            ImGui::DragFloat("Camera Smoothing", &aladdin->cameraSmoothing, 0.1f, 0.0f, 20.0f);

            ImGui::Separator();
            ImGui::Text("Physics State:");
            ImGui::DragFloat3("Velocity", &aladdin->velocity[0], 0.1f);
            if(Entity* owner = aladdin->getOwner()) {
                if(auto* mov = owner->getComponent<MovementComponent>()) {
                    ImGui::DragFloat3("Movement linearVelocity", &mov->linearVelocity[0], 0.1f);
                }
            }

            if(ImGui::Button("Reset Position")) {
                if(Entity* owner = aladdin->getOwner()) {
                    owner->localTransform.position = {0, 0, 0};
                    aladdin->velocity = {0, 0, 0};
                    if(auto* mov = owner->getComponent<MovementComponent>()) {
                        mov->linearVelocity = {0, 0, 0};
                    }
                }
            }

            ImGui::End();
        }
    };
}
