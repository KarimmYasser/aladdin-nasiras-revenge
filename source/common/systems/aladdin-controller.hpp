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
#include "../components/animator-component.hpp"
#include "../components/skinned-mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include "../application.hpp"
#include "../physics/physics-system.hpp"
#include "../audio/audio-system.hpp"
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
                auto& mouse = app->getMouse();

                // ─── 0. Mouse orbit: update camera orbit yaw/pitch ───
                if (mouse.isPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                    glm::vec2 delta = mouse.getMouseDelta();
                    aladdin->cameraOrbitYaw   -= delta.x * aladdin->mouseSensitivity;
                    aladdin->cameraOrbitPitch -= delta.y * aladdin->mouseSensitivity;
                    // Clamp pitch to avoid flipping
                    aladdin->cameraOrbitPitch = glm::clamp(
                        aladdin->cameraOrbitPitch,
                        -glm::half_pi<float>() * 0.85f,
                         glm::half_pi<float>() * 0.35f);
                }

                // ─── 1. Camera-relative movement from WASD ───
                // Movement is relative to the camera's orbit yaw (horizontal facing)
                float inputFwd = 0.0f, inputRight = 0.0f;
                if(keyboard.isPressed(GLFW_KEY_W)) inputFwd   += 1.0f;
                if(keyboard.isPressed(GLFW_KEY_S)) inputFwd   -= 1.0f;
                if(keyboard.isPressed(GLFW_KEY_A)) inputRight -= 1.0f;
                if(keyboard.isPressed(GLFW_KEY_D)) inputRight += 1.0f;

                // Camera forward/right on XZ plane (from orbit yaw, NOT from character yaw)
                const float orbYaw = aladdin->cameraOrbitYaw;
                const glm::vec3 camFwd   = glm::vec3(-glm::sin(orbYaw), 0.0f, -glm::cos(orbYaw));
                const glm::vec3 camRight = glm::vec3( glm::cos(orbYaw), 0.0f, -glm::sin(orbYaw));

                glm::vec3 moveDir = camFwd * inputFwd + camRight * inputRight;
                const bool hasMoveInput = glm::length(moveDir) > 0.001f;
                if (hasMoveInput) {
                    moveDir = glm::normalize(moveDir);
                    // Smoothly rotate character to face the movement direction (exponential smoothing)
                    const float targetYaw = glm::atan(moveDir.x, moveDir.z);
                    float diff = targetYaw - aladdin->facingYaw;
                    while (diff >  glm::pi<float>()) diff -= 2.0f * glm::pi<float>();
                    while (diff < -glm::pi<float>()) diff += 2.0f * glm::pi<float>();
                    const float smoothFactor = 1.0f - glm::exp(-aladdin->rotationSpeed * deltaTime);
                    aladdin->facingYaw += diff * smoothFactor;
                    while (aladdin->facingYaw >  glm::pi<float>()) aladdin->facingYaw -= 2.0f * glm::pi<float>();
                    while (aladdin->facingYaw < -glm::pi<float>()) aladdin->facingYaw += 2.0f * glm::pi<float>();
                }
                // Apply visual rotation — facingYaw directly, no 180° offset
                entity->localTransform.rotation.x = 0.0f;
                entity->localTransform.rotation.y = aladdin->facingYaw;
                entity->localTransform.rotation.z = 0.0f;

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
                        // Calculate the distance from the center to the bottom of the collider
                        float halfHeight = 0.0f;
                        if (auto* collider = entity->getComponent<ColliderComponent>()) {
                            switch (collider->shape) {
                                case ColliderShape::Box:     halfHeight = collider->halfExtents.y; break;
                                case ColliderShape::Sphere:  halfHeight = collider->radius; break;
                                case ColliderShape::Capsule: halfHeight = (collider->height * 0.5f) + collider->radius; break;
                            }
                        }

                        // Start the raycast exactly at the feet (or slightly above to avoid clipping)
                        // and cast only a small distance down (0.25m)
                        const glm::vec3 origin = entity->localTransform.position - glm::vec3(0.0f, halfHeight - 0.1f, 0.0f);
                        const float rayLength = 0.25f; 
                        
                        RaycastHit groundHit = physicsWorld.raycast(origin, glm::vec3(0.0f, -1.0f, 0.0f), rayLength);
                        
                        // Ignore the player's own entity and their sword hitbox
                        Entity* sword = (swordHitboxes.count(entity) > 0) ? swordHitboxes[entity] : nullptr;
                        groundedFromRaycast = groundHit.hasHit && groundHit.entity && 
                                             groundHit.entity != entity && 
                                             groundHit.entity != sword &&
                                             groundHit.normal.y >= 0.5f;
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
                    AudioSystem::instance().playSound("assets/audio/attack.wav");
                }

                if(aladdin->isAttacking) {
                    aladdin->attackTimer -= deltaTime;
                    if(aladdin->attackTimer <= 0.0f) {
                        aladdin->isAttacking = false;
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
         * Run after PhysicsSystem::update so overlap events and player position match the stepped world.
         * Handles sword hits and third-person camera follow.
         */
        void postPhysicsUpdate(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            for (auto entity : world->getEntities()) {
                AladdinControllerComponent* aladdin = entity->getComponent<AladdinControllerComponent>();
                if (!aladdin || aladdin->lives <= 0) continue;

                Entity* swordHitbox = getOrCreateSwordHitbox(world, entity);
                updateSwordHitboxTransform(entity, swordHitbox);

                if (aladdin->isAttacking) {
                    for (auto other : world->getEntities()) {
                        if (other == entity || other == swordHitbox) continue;

                        bool swordOverlap = false;
                        if (physicsSystem) {
                            auto& physicsWorld = physicsSystem->getPhysicsWorld();
                            swordOverlap = physicsWorld.hasAnyInteraction(swordHitbox, other, true);
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
                                AudioSystem::instance().playSound("assets/audio/death.wav");
                            } else {
                                AudioSystem::instance().playSound("assets/audio/hit.wav");
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

                // ── Animation state machine ────────────────────────────────────────
                // Drive whichever animation component is present on this entity.
                // Prefer SkinnedMeshRendererComponent; fall back to the legacy AnimatorComponent.
                Animator* animPtr = nullptr;
                if (auto* smr = entity->getComponent<SkinnedMeshRendererComponent>())
                    animPtr = &smr->animator;
                else if (auto* anim = entity->getComponent<AnimatorComponent>())
                    animPtr = &anim->animator;

                if (animPtr) {
                    std::string targetClip = "idle";
                    bool loop = true;

                    if (aladdin->isAttacking) {
                        targetClip = "kick";
                        loop = false;
                    } else if (!aladdin->isGrounded) {
                        targetClip = "jump";
                        loop = false;
                    } else if (glm::length(glm::vec2(aladdin->velocity.x,
                                                      aladdin->velocity.z)) > 0.1f) {
                        targetClip = "walk";
                    }

                    // Fallback: If target clip isn't found, try "walk", then first available clip
                    if (!animPtr->hasClip(targetClip)) {
                        if (animPtr->hasClip("walk")) {
                            targetClip = "walk";
                        } else if (!animPtr->getClips().empty()) {
                            targetClip = animPtr->getClips().begin()->first;
                        }
                    }

                    animPtr->play(targetClip, loop);
                }

            }
            updateFollowCamera(world, physicsSystem, deltaTime);
        }

        /**
         * @brief Spring-arm style third-person camera (like UE4's Camera Boom).
         *
         * The camera orbits behind the player based on the character's facing yaw.
         * A raycast from the focus point (player + height offset) toward the desired
         * camera position detects walls. If a wall is hit, the arm shortens so the
         * camera sits just in front of the wall surface. When no wall is blocking,
         * the arm smoothly recovers to its full configured length.
         *
         * Position and rotation are exponentially smoothed for a cinematic feel.
         */
        void updateFollowCamera(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            if (!world || !app) return;

            for (auto entity : world->getEntities()) {
                AladdinControllerComponent* aladdin = entity->getComponent<AladdinControllerComponent>();
                if (!aladdin || aladdin->lives <= 0 || !aladdin->enableCameraFollow) continue;

                Entity* cameraEntity = nullptr;
                for (auto e : world->getEntities()) {
                    if (e->getComponent<CameraComponent>()) {
                        cameraEntity = e;
                        break;
                    }
                }
                if (!cameraEntity) continue;

                // ── Focus point: the point the camera looks at ──
                const glm::vec3& playerPos = entity->localTransform.position;
                const glm::vec3 focusPoint = playerPos + glm::vec3(0.0f, aladdin->cameraFocusHeight, 0.0f);

                if (aladdin->cameraMode == AladdinCameraMode::ThirdPerson) {
                    // ── Orbit camera using mouse-driven yaw/pitch ──
                    const float orbYaw   = aladdin->cameraOrbitYaw;
                    const float orbPitch = aladdin->cameraOrbitPitch;

                    // Spherical-to-cartesian: arm direction from focus point to camera
                    const glm::vec3 armDir = glm::normalize(glm::vec3(
                        glm::cos(orbPitch) *  glm::sin(orbYaw),
                        -glm::sin(orbPitch),
                        glm::cos(orbPitch) *  glm::cos(orbYaw)));

                    float desiredDist = aladdin->cameraArmLength;

                    // ── Wall collision via raycast ──
                    if (physicsSystem) {
                        RaycastHit hit = physicsSystem->getPhysicsWorld().raycast(
                            focusPoint, armDir, desiredDist + 0.5f);

                        if (hit.hasHit && hit.entity && hit.entity != entity) {
                            float wallDist = glm::max(
                                hit.distance - aladdin->cameraWallOffset,
                                aladdin->cameraArmMinDist);
                            desiredDist = glm::min(desiredDist, wallDist);
                        }
                    }

                    // ── Smooth arm distance: snap in, ease out ──
                    if (desiredDist < aladdin->currentArmDist) {
                        aladdin->currentArmDist = desiredDist;
                    } else {
                        aladdin->currentArmDist += (desiredDist - aladdin->currentArmDist)
                            * glm::min(1.0f, aladdin->cameraArmRecoverSpeed * deltaTime);
                    }

                    // ── Final camera position ──
                    const glm::vec3 targetPosition = focusPoint + armDir * aladdin->currentArmDist;

                    // ── Camera rotation: look from camera toward focus ──
                    const glm::vec3 lookDir = glm::normalize(focusPoint - targetPosition);
                    const float targetPitch = glm::asin(glm::clamp(lookDir.y, -1.0f, 1.0f));
                    const float targetYaw = glm::atan(-lookDir.x, -lookDir.z);
                    const glm::vec3 targetRotation = {targetPitch, targetYaw, 0.0f};

                    // ── Exponential smoothing ──
                    if (aladdin->cameraSmoothing > 0.0f) {
                        const float camDt = glm::min(deltaTime, 0.05f);
                        const float factor = 1.0f - glm::exp(-aladdin->cameraSmoothing * camDt);

                        cameraEntity->localTransform.position = glm::mix(
                            cameraEntity->localTransform.position, targetPosition, factor);

                        glm::vec3& r = cameraEntity->localTransform.rotation;
                        r.x += (targetRotation.x - r.x) * factor;
                        float dyaw = targetRotation.y - r.y;
                        while (dyaw >  glm::pi<float>()) dyaw -= 2.0f * glm::pi<float>();
                        while (dyaw < -glm::pi<float>()) dyaw += 2.0f * glm::pi<float>();
                        r.y += dyaw * factor;
                        r.z = 0.0f;
                    } else {
                        cameraEntity->localTransform.position = targetPosition;
                        cameraEntity->localTransform.rotation = targetRotation;
                    }
                } else {
                    // ── First-person mode ──
                    const float fpYaw = aladdin->facingYaw;
                    const glm::vec3 fpForward = glm::normalize(glm::vec3(-glm::sin(fpYaw), 0.0f, -glm::cos(fpYaw)));
                    const glm::vec3 fpRight   = glm::normalize(glm::cross(fpForward, glm::vec3(0.0f, 1.0f, 0.0f)));
                    const glm::vec3 fpUp(0.0f, 1.0f, 0.0f);
                    const glm::vec3 targetPosition = playerPos
                        + fpRight   * aladdin->firstPersonCameraOffset.x
                        + fpUp      * aladdin->firstPersonCameraOffset.y
                        + fpForward * aladdin->firstPersonCameraOffset.z;
                    const glm::vec3 targetRotation = {aladdin->firstPersonPitch, fpYaw, 0.0f};

                    cameraEntity->localTransform.position = targetPosition;
                    cameraEntity->localTransform.rotation = targetRotation;
                }

                // Keep mesh rotation clean
                entity->localTransform.rotation.x = 0.0f;
                entity->localTransform.rotation.y = aladdin->facingYaw;
                entity->localTransform.rotation.z = 0.0f;

                // Toggle visibility for first-person camera: prefer new component, fall back to legacy
                if (auto* smr = entity->getComponent<SkinnedMeshRendererComponent>()) {
                    smr->visible = (aladdin->cameraMode != AladdinCameraMode::FirstPerson);
                } else if (auto* meshRenderer = entity->getComponent<MeshRendererComponent>()) {
                    meshRenderer->visible = (aladdin->cameraMode != AladdinCameraMode::FirstPerson);
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
            int camMode = (aladdin->cameraMode == AladdinCameraMode::FirstPerson) ? 1 : 0;
            if (ImGui::Combo("Camera mode", &camMode, "Third person\0First person\0")) {
                aladdin->cameraMode = camMode ? AladdinCameraMode::FirstPerson : AladdinCameraMode::ThirdPerson;
            }
            ImGui::DragFloat3("First-person offset", &aladdin->firstPersonCameraOffset[0], 0.02f);
            ImGui::DragFloat("First-person pitch (rad)", &aladdin->firstPersonPitch, 0.01f, -1.2f, 1.2f);
            ImGui::DragFloat("Camera Smoothing (0=snap)", &aladdin->cameraSmoothing, 0.1f, 0.0f, 20.0f);
            ImGui::Text("Orbit Camera (Right-click drag):");
            ImGui::DragFloat("Mouse Sensitivity", &aladdin->mouseSensitivity, 0.0005f, 0.001f, 0.02f);
            ImGui::Value("Orbit Yaw", aladdin->cameraOrbitYaw);
            ImGui::Value("Orbit Pitch", aladdin->cameraOrbitPitch);
            ImGui::Text("Spring Arm:");
            ImGui::DragFloat("Arm Length", &aladdin->cameraArmLength, 0.1f, 1.0f, 20.0f);
            ImGui::DragFloat("Arm Min Dist", &aladdin->cameraArmMinDist, 0.05f, 0.2f, 5.0f);
            ImGui::DragFloat("Wall Offset", &aladdin->cameraWallOffset, 0.05f, 0.0f, 2.0f);
            ImGui::DragFloat("Recover Speed", &aladdin->cameraArmRecoverSpeed, 0.1f, 0.5f, 20.0f);
            ImGui::DragFloat("Focus Height", &aladdin->cameraFocusHeight, 0.05f, 0.0f, 5.0f);
            ImGui::Value("Current Arm Dist", aladdin->currentArmDist);

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