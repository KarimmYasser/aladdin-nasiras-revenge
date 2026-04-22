#pragma once

#include "../ecs/world.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/camera.hpp"
#include "../components/enemy.hpp"
#include "../components/breakable.hpp"
#include "../components/collectible.hpp"
#include "../components/rigid-body.hpp"
#include "../components/collider.hpp"
#include "../components/mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include "../application.hpp"
#include "../physics/physics-system.hpp"
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace our {

    /**
     * @brief System that handles character movement, jumping, and states for Aladdin.
     * 
     * It reads keyboard input to update the entity's position, applies "fake" gravity
     * until a physics system is available, and manages character rotation.
     */
    class AladdinControllerSystem {
        Application* app; // Pointer to the application for input handling
        std::unordered_map<Entity*, Entity*> swordHitboxes;

    public:
        // Initialize with the application pointer
        void enter(Application* app) {
            this->app = app;
        }

        static bool wasHitInCurrentAttack(AladdinControllerComponent* aladdin, Entity* target) {
            return std::find(aladdin->hitEntities.begin(), aladdin->hitEntities.end(), target) != aladdin->hitEntities.end();
        }

        static void markHitInCurrentAttack(AladdinControllerComponent* aladdin, Entity* target) {
            aladdin->hitEntities.push_back(target);
        }

        Entity* getOrCreateSwordHitbox(World* world, Entity* player) {
            auto it = swordHitboxes.find(player);
            if (it != swordHitboxes.end() && it->second) {
                return it->second;
            }

            Entity* hitbox = world->add();
            hitbox->name = player->name + "_SwordHitbox";

            auto* rb = hitbox->addComponent<RigidBodyComponent>();
            rb->type = RigidBodyType::Kinematic;
            rb->mass = 0.0f;
            rb->useGravity = false;
            rb->lockRotation = true;

            auto* collider = hitbox->addComponent<ColliderComponent>();
            collider->shape = ColliderShape::Box;
            collider->halfExtents = glm::vec3(0.5f, 0.6f, 0.9f);
            collider->isTrigger = true;

            swordHitboxes[player] = hitbox;
            return hitbox;
        }

        static void updateSwordHitboxTransform(Entity* player, Entity* swordHitbox) {
            const float yaw = player->localTransform.rotation.y;
            const glm::vec3 forward = glm::normalize(glm::vec3(glm::sin(yaw), 0.0f, glm::cos(yaw)));
            const glm::vec3 offset = forward * 1.1f + glm::vec3(0.0f, 0.9f, 0.0f);

            swordHitbox->localTransform.position = player->localTransform.position + offset;
            swordHitbox->localTransform.rotation = glm::vec3(0.0f, yaw, 0.0f);
            swordHitbox->localTransform.scale = glm::vec3(1.0f);
        }

        /**
         * @brief Updates every entity in the world containing a AladdinControllerComponent.
         * 
         * @param world The scene world containing entities.
         * @param deltaTime The time elapsed since the last frame.
         */
        void update(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            std::vector<Entity*> players;
            players.reserve(world->getEntities().size());
            for (auto candidate : world->getEntities()) {
                if (candidate->getComponent<AladdinControllerComponent>()) {
                    players.push_back(candidate);
                }
            }

            for(auto entity : players){
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

                const bool hasMoveInput = glm::length(moveDir) > 0.001f;
                if(hasMoveInput) {
                    moveDir = glm::normalize(moveDir);

                    // Rotate entity to face the movement direction
                    float targetYaw = glm::atan(moveDir.x, moveDir.z);
                    entity->localTransform.rotation.y = targetYaw;
                }

                auto* rbComp = entity->getComponent<RigidBodyComponent>();
                const bool hasPhysicsBody = physicsSystem && rbComp && rbComp->bodyHandle;

                Entity* swordHitbox = getOrCreateSwordHitbox(world, entity);
                updateSwordHitboxTransform(entity, swordHitbox);

                // 3. Handle gravity, movement and jumping via real physics (if body exists)
                if (hasPhysicsBody) {
                    auto& physicsWorld = physicsSystem->getPhysicsWorld();

                    bool groundedFromContacts = physicsWorld.isGrounded(entity, 0.5f);
                    bool groundedFromRaycast = false;
                    if (!groundedFromContacts) {
                        float probeDistance = 1.15f;
                        if (auto* collider = entity->getComponent<ColliderComponent>()) {
                            switch (collider->shape) {
                                case ColliderShape::Box:
                                    probeDistance = collider->halfExtents.y + 0.25f;
                                    break;
                                case ColliderShape::Sphere:
                                    probeDistance = collider->radius + 0.25f;
                                    break;
                                case ColliderShape::Capsule:
                                    probeDistance = (collider->height * 0.5f) + collider->radius + 0.25f;
                                    break;
                            }
                        }

                        const glm::vec3 origin = entity->localTransform.position + glm::vec3(0.0f, 0.05f, 0.0f);
                        RaycastHit groundHit = physicsWorld.raycast(origin, glm::vec3(0.0f, -1.0f, 0.0f), probeDistance);
                        groundedFromRaycast = groundHit.hasHit && groundHit.entity && groundHit.entity != entity && groundHit.normal.y >= 0.5f;
                    }

                    aladdin->isGrounded = groundedFromContacts || groundedFromRaycast;

                    glm::vec3 currentVelocity = physicsWorld.getLinearVelocity(entity);
                    glm::vec3 targetVelocity = currentVelocity;
                    targetVelocity.x = hasMoveInput ? moveDir.x * aladdin->speed : 0.0f;
                    targetVelocity.z = hasMoveInput ? moveDir.z * aladdin->speed : 0.0f;

                    if(keyboard.justPressed(GLFW_KEY_SPACE) && aladdin->isGrounded) {
                        targetVelocity.y = aladdin->jumpForce;
                        aladdin->isGrounded = false;
                    }

                    physicsWorld.setLinearVelocity(entity, targetVelocity);
                    aladdin->velocity = targetVelocity;
                } else {
                    // Fallback for scenes that still do not have a rigid body setup yet
                    if(hasMoveInput) {
                        entity->localTransform.position += moveDir * aladdin->speed * deltaTime;
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
                    
                    // Simple Combat Test: consume physics interactions for sword hits.
                    // This assumes entities that can be hit have colliders configured.
                    for(auto other : world->getEntities()){
                        if(other == entity || other == swordHitbox) continue;

                        bool swordOverlap = false;
                        if (physicsSystem) {
                            auto& physicsWorld = physicsSystem->getPhysicsWorld();
                            swordOverlap = physicsWorld.hasAnyInteraction(swordHitbox, other, false);
                        }
                        if(!swordOverlap) continue;

                        // Check for Target objects (from original mock)
                        if(other->name.find("Target") != std::string::npos){
                            if (wasHitInCurrentAttack(aladdin, other)) continue;

                            markHitInCurrentAttack(aladdin, other);
                            std::cout << "Sword Hit: " << other->name << "!" << std::endl;
                            glm::vec3 dir = other->localTransform.position - entity->localTransform.position;
                            if(glm::length(dir) > 0.0001f) {
                                other->localTransform.position += glm::normalize(dir) * 0.1f;
                            }
                        }

                        // Check for Real Enemies
                        EnemyComponent* enemy = other->getComponent<EnemyComponent>();
                        if(enemy && enemy->currentState != EnemyComponent::State::DEAD) {
                            if(wasHitInCurrentAttack(aladdin, other)) continue;

                            enemy->health -= 25; // Aladdin deals 25 damage per hit
                            markHitInCurrentAttack(aladdin, other); // Mark this enemy as hit
                            std::cout << "[AladdinSystem] Hit " << other->name << "! Enemy Health: " << enemy->health << std::endl;

                            if(enemy->health <= 0) {
                                enemy->currentState = EnemyComponent::State::DEAD;
                                std::cout << "[AladdinSystem] " << other->name << " defeated!" << std::endl;
                            }
                        }

                        // Check for Breakable Props (Pots)
                        BreakableComponent* breakable = other->getComponent<BreakableComponent>();
                        if(breakable) {
                            if(wasHitInCurrentAttack(aladdin, other)) continue;

                            markHitInCurrentAttack(aladdin, other); // Mark this breakable as hit
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

                                // Add MeshRenderer for loot
                                auto mr = loot->addComponent<MeshRendererComponent>();
                                mr->mesh = AssetLoader<Mesh>::get("cube");
                                mr->material = AssetLoader<Material>::get("loot_mat");

                                    // Add Collectible component
                                    auto coll = loot->addComponent<CollectibleComponent>();
                                    if(lootEntry.type == "coin") coll->type = CollectibleComponent::Type::COIN;
                                    else if(lootEntry.type == "gem") coll->type = CollectibleComponent::Type::GEM;
                                    else if(lootEntry.type == "apple") coll->type = CollectibleComponent::Type::APPLE;
                                    coll->value = lootEntry.value;

                                    // Add physics trigger so collectible system can detect pickup
                                    auto rb = loot->addComponent<RigidBodyComponent>();
                                    rb->type = RigidBodyType::Static;
                                    rb->mass = 0.0f;
                                    rb->useGravity = false;

                                    auto lootCollider = loot->addComponent<ColliderComponent>();
                                    lootCollider->shape = ColliderShape::Sphere;
                                    lootCollider->radius = 0.5f;
                                    lootCollider->isTrigger = true;
                                }

                            // Mark the pot for removal
                            world->markForRemoval(other);
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
