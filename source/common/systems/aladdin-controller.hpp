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
#include "../components/projectile.hpp"
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
            collider->halfExtents = glm::vec3(0.8f, 1.2f, 1.2f);
            collider->isTrigger = true;

            swordHitboxes[player] = hitbox;
            return hitbox;
        }

        static void updateSwordHitboxTransform(Entity* player, Entity* swordHitbox) {
            const float yaw = player->localTransform.rotation.y;
            const glm::vec3 forward = glm::normalize(glm::vec3(glm::sin(yaw), 0.0f, glm::cos(yaw)));
            const glm::vec3 offset = forward * 1.1f + glm::vec3(0.0f, 0.0f, 0.0f);

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

                // Sync sensitivity with global settings
                if (app->getConfig().contains("game")) {
                    aladdin->mouseSensitivity = app->getConfig()["game"].value("mouseSensitivity", aladdin->mouseSensitivity);
                }

                auto& keyboard = app->getKeyboard();
                auto& mouse = app->getMouse();

                // ─── 0. Mouse orbit or Crosshair Move ───
                if (aladdin->isAiming) {
                    // Hide OS cursor but keep tracking active
                    glfwSetInputMode(app->getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                    
                    // Sync our aimOffset with the actual mouse position relative to center
                    glm::vec2 mousePos = mouse.getMousePosition();
                    glm::vec2 windowSize = app->getWindowSize();
                    aladdin->aimOffset.x = mousePos.x - (windowSize.x * 0.5f);
                    aladdin->aimOffset.y = mousePos.y - (windowSize.y * 0.5f);
                } else if (mouse.isPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                    // Lock cursor for orbit
                    glfwSetInputMode(app->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    
                    // Normal camera rotation
                    glm::vec2 delta = mouse.getMouseDelta();
                    aladdin->cameraOrbitYaw   -= delta.x * aladdin->mouseSensitivity;
                    aladdin->cameraOrbitPitch -= delta.y * aladdin->mouseSensitivity;
                    aladdin->cameraOrbitPitch = glm::clamp(
                        aladdin->cameraOrbitPitch,
                        -glm::half_pi<float>() * 0.85f,
                         glm::half_pi<float>() * 0.35f);
                } else {
                    // Default state: ensure cursor is locked if not aiming/orbiting 
                    // (depending on game design, but usually for TPS it stays locked)
                    glfwSetInputMode(app->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
                if(keyboard.isPressed(GLFW_KEY_R) && aladdin->appleCount > 0) {
                    if (!aladdin->isAiming) {
                        aladdin->isAiming = true;
                        // Center the actual mouse cursor at start of aiming
                        glm::vec2 windowSize = app->getWindowSize();
                        glfwSetCursorPos(app->getWindow(), windowSize.x * 0.5, windowSize.y * 0.5);
                        aladdin->aimOffset = {0, 0};
                    }
                } else if (aladdin->isAiming) {
                    // KEY RELEASED: Throw the apple!
                    aladdin->isAiming = false;
                    aladdin->isThrowing = true;
                    aladdin->throwTimer = 0.3f;
                    aladdin->appleCount--;
                    
                    // --- SPAWN PHYSICAL APPLE ---
                    Entity* apple = world->add();
                    apple->name = "ThrownApple";
                    
                    // Find the camera's forward direction to aim correctly
                    Entity* cameraEntity = nullptr;
                    for (auto e : world->getEntities()) {
                        if (e->getComponent<CameraComponent>()) {
                            cameraEntity = e;
                            break;
                        }
                    }

                    // Calculate direction: from camera forward + crosshair offset
                    glm::vec3 throwDir;
                    if (cameraEntity) {
                        const float cyaw = cameraEntity->localTransform.rotation.y;
                        const float cpitch = cameraEntity->localTransform.rotation.x;
                        
                        // Camera base vectors
                        glm::vec3 camFwd = glm::normalize(glm::vec3(
                            -glm::cos(cpitch) * glm::sin(cyaw),
                             glm::sin(cpitch),
                            -glm::cos(cpitch) * glm::cos(cyaw)
                        ));
                        glm::vec3 camRight = glm::normalize(glm::cross(camFwd, glm::vec3(0, 1, 0)));
                        glm::vec3 camUp = glm::normalize(glm::cross(camRight, camFwd));

                        // Apply aim offset (normalized by screen sensitivity)
                        float sens = 0.002f; 
                        throwDir = glm::normalize(camFwd + (aladdin->aimOffset.x * sens * camRight) - (aladdin->aimOffset.y * sens * camUp));
                    } else {
                        const float pyaw = aladdin->facingYaw;
                        throwDir = glm::normalize(glm::vec3(-glm::sin(pyaw), 0.2f, -glm::cos(pyaw)));
                    }

                    // Position at Aladdin's hands
                    const float pyaw = aladdin->facingYaw;
                    const glm::vec3 playerFwd = glm::vec3(-glm::sin(pyaw), 0.0f, -glm::cos(pyaw));
                    apple->localTransform.position = entity->localTransform.position + glm::vec3(0, 1.3f, 0) + playerFwd * 0.8f;
                    apple->localTransform.scale = glm::vec3(1.2f);
                    
                    auto* mr = apple->addComponent<MeshRendererComponent>();
                    mr->mesh = AssetLoader<Mesh>::get("apple_mesh");
                    mr->material = AssetLoader<Material>::get("lit-apple");
                    
                    auto* projectile = apple->addComponent<ProjectileComponent>();
                    projectile->owner = entity;
                    projectile->damage = 34.0f;
                    
                    auto* rb = apple->addComponent<RigidBodyComponent>();
                    rb->type = RigidBodyType::Dynamic;
                    rb->useGravity = true;
                    
                    auto* col = apple->addComponent<ColliderComponent>();
                    col->shape = ColliderShape::Sphere;
                    col->radius = 0.25f;
                    col->isTrigger = true;

                    // Set Initial Velocity in aiming direction
                    glm::vec3 throwVel = throwDir * 25.0f; // Fast throw
                    if (throwVel.y < 3.0f) throwVel.y += 3.0f; // Slight upward arc
                    rb->velocity = throwVel;
                }

                if(aladdin->isThrowing) {
                    aladdin->throwTimer -= deltaTime;
                    if(aladdin->throwTimer <= 0.0f) {
                        aladdin->isThrowing = false;
                    }
                }

            }
        }

        /**
         * Run after PhysicsSystem::update so overlap events and player position match the stepped world.
         * Handles sword hits and third-person camera follow.
         */
        void postPhysicsUpdate(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            if (!world) return;

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

                        // Check for Enemies
                        EnemyComponent* enemy = other->getComponent<EnemyComponent>();
                        if(enemy && enemy->currentState != EnemyComponent::State::DEAD) {
                            if(wasHitInCurrentAttack(aladdin, other)) continue;
                            enemy->health -= 25;
                            markHitInCurrentAttack(aladdin, other);
                            if(enemy->health <= 0) {
                                enemy->currentState = EnemyComponent::State::DEAD;
                                AudioSystem::instance().playSound("assets/audio/death.wav");
                            } else {
                                AudioSystem::instance().playSound("assets/audio/hit.wav");
                            }
                        }

                        // Check for Breakable Props
                        BreakableComponent* breakable = other->getComponent<BreakableComponent>();
                        if(breakable) {
                            if(wasHitInCurrentAttack(aladdin, other)) continue;
                            markHitInCurrentAttack(aladdin, other);
                            
                            struct LootRequest {
                                glm::vec3 position;
                                LootEntry entry;
                            };
                            std::vector<LootRequest> requests;
                            for(size_t i = 0; i < breakable->lootItems.size(); ++i) {
                                float angle = ((float)i / (float)breakable->lootItems.size()) * 2.0f * glm::pi<float>();
                                float radius = 4.0f; // Wider scatter to prevent instant pickup
                                glm::vec3 scatterOffset = glm::vec3(glm::cos(angle) * radius, 1.2f, glm::sin(angle) * radius);
                                requests.push_back({other->localTransform.position + scatterOffset, breakable->lootItems[i]});
                            }

                            for(const auto& req : requests) {
                                Entity* loot = world->add();
                                loot->name = "Dropped_" + req.entry.type;
                                loot->localTransform.position = req.position;
                                
                                auto mr = loot->addComponent<MeshRendererComponent>();
                                auto coll = loot->addComponent<CollectibleComponent>();
                                coll->pickupDelay = 0.6f; // Delay pickup so they can be seen spawning
                                
                                if(req.entry.type == "coin") {
                                    mr->mesh = AssetLoader<Mesh>::get("coin_mesh");
                                    mr->material = AssetLoader<Material>::get("coin-mat");
                                    loot->localTransform.scale = glm::vec3(4.6f);
                                    coll->type = CollectibleComponent::Type::COIN;
                                } else if(req.entry.type == "apple") {
                                    mr->mesh = AssetLoader<Mesh>::get("apple_mesh");
                                    mr->material = AssetLoader<Material>::get("lit-apple");
                                    loot->localTransform.scale = glm::vec3(1.3f);
                                    coll->type = CollectibleComponent::Type::APPLE;
                                } else if(req.entry.type == "health") {
                                    mr->mesh = AssetLoader<Mesh>::get("health_bottle_mesh");
                                    mr->material = AssetLoader<Material>::get("health-bottle-mat");
                                    loot->localTransform.scale = glm::vec3(8.0f);
                                    coll->type = CollectibleComponent::Type::HEALTH;
                                } else {
                                    // Default/Gem case
                                    mr->mesh = AssetLoader<Mesh>::get("cube");
                                    mr->material = AssetLoader<Material>::get("coin-mat");
                                    loot->localTransform.scale = glm::vec3(0.5f);
                                    coll->type = CollectibleComponent::Type::GEM;
                                }
                                coll->value = req.entry.value;

                                auto rb = loot->addComponent<RigidBodyComponent>();
                                rb->type = RigidBodyType::Static;
                                auto lootCollider = loot->addComponent<ColliderComponent>();
                                lootCollider->shape = ColliderShape::Sphere;
                                lootCollider->radius = 0.5f;
                                lootCollider->isTrigger = true;
                            }
                            // Mark the pot for removal and destroy its physics body immediately
                            if (physicsSystem) {
                                physicsSystem->getPhysicsWorld().destroyRigidBody(other);
                            }
                            world->markForRemoval(other);
                            AudioSystem::instance().playSound("assets/audio/potBreak.mp3");
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
                        animPtr->play(targetClip, loop, 1.6f); // Faster kick
                        continue; // Skip the default play call below
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