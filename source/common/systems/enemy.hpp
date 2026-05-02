#pragma once

#include "../ecs/world.hpp"
#include "../components/enemy.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/rigid-body.hpp"
#include "../physics/physics-system.hpp"
#include "../audio/audio-system.hpp"
#include <glm/glm.hpp>
#include <cfloat>
#include <iostream>
#include <imgui.h>
#include "../components/camera.hpp"
#include "../components/skinned-mesh-renderer.hpp"
#include "../components/animator-component.hpp"

namespace our {

    class EnemySystem {
    private:
        /**
         * @brief Computes if an enemy is grounded using physics raycast and contact normals.
         * 
         * Accounts for collider center offset to accurately determine ground contact.
         */
        static bool computeGroundedFromPhysics(Entity* entity, PhysicsWorld& physicsWorld) {
            constexpr float kMinUpDotContact = 0.50f;
            constexpr float kProbeStartAboveFeet = 0.12f;
            constexpr float kRayLength = 0.42f;
            constexpr float kMinUpDotRay = 0.50f;

            const bool fromContacts = physicsWorld.isGrounded(entity, kMinUpDotContact);

            float halfHeight = 0.0f;
            float centerOffsetY = 0.0f;
            if (auto* collider = entity->getComponent<ColliderComponent>()) {
                centerOffsetY = collider->centerOffset.y;
                switch (collider->shape) {
                    case ColliderShape::Box:     halfHeight = collider->halfExtents.y; break;
                    case ColliderShape::Sphere:  halfHeight = collider->radius; break;
                    case ColliderShape::Capsule: halfHeight = (collider->height * 0.5f) + collider->radius; break;
                    default: break;
                }
            }

            // Probe from slightly above the collider's bottom (feet), accounting for collider center offset.
            const float feetY = entity->localTransform.position.y + centerOffsetY - halfHeight;
            const glm::vec3 origin(entity->localTransform.position.x, feetY + kProbeStartAboveFeet, entity->localTransform.position.z);
            const RaycastHit groundHit = physicsWorld.raycast(origin, glm::vec3(0.0f, -1.0f, 0.0f), kRayLength);
            const bool fromRay = groundHit.hasHit && groundHit.entity &&
                groundHit.entity != entity &&
                groundHit.normal.y >= kMinUpDotRay;

            return fromContacts || fromRay;
        }

    public:
        void update(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            // Find Aladdin (Player)
            Entity* playerEntity = nullptr;
            AladdinControllerComponent* playerController = nullptr;
            for(auto entity : world->getEntities()) {
                playerController = entity->getComponent<AladdinControllerComponent>();
                if(playerController) {
                    playerEntity = entity;
                    break;
                }
            }

            if(!playerEntity || !playerController || playerController->lives <= 0) return;

            for(auto entity : world->getEntities()) {
                EnemyComponent* enemy = entity->getComponent<EnemyComponent>();
                if(!enemy) continue;

                // Update Health Bar Timer
                if (enemy->healthBarTimer > 0.0f) {
                    enemy->healthBarTimer -= deltaTime;
                }

                // ── Animation State Sync ──
                // If the enemy has a SkinnedMeshRenderer (new pattern) or AnimatorComponent (legacy),
                // we sync the animation clip to the AI state.
                // This is done at the top so it runs even if the AI logic early-exits (e.g. for DEAD state).
                Animator* animator = nullptr;
                if (auto* smr = entity->getComponent<SkinnedMeshRendererComponent>()) animator = &smr->animator;
                else if (auto* anim = entity->getComponent<AnimatorComponent>()) animator = &anim->animator;

                if (animator) {
                    std::string targetClip = "idle";
                    float playbackSpeed = 1.0f;
                    bool loop = true;

                    if (enemy->currentState == EnemyComponent::State::DEAD) {
                        targetClip = "death";
                        playbackSpeed = 1.0f;
                        loop = false;
                    } else if (enemy->idleTimer > 0.0f) {
                        targetClip = "idle";
                        playbackSpeed = 1.0f;
                    } else {
                        switch (enemy->currentState) {
                            case EnemyComponent::State::PATROL:
                                if (enemy->waypoints.size() > 1) {
                                    targetClip = "walk";
                                    playbackSpeed = 1.0f;
                                } else {
                                    targetClip = "idle";
                                    playbackSpeed = 1.0f;
                                }
                                break;
                            case EnemyComponent::State::CHASE:
                                targetClip = "walk";
                                playbackSpeed = 1.5f;
                                break;
                            case EnemyComponent::State::ATTACK:
                                targetClip = "attack";
                                playbackSpeed = 1.0f;
                                break;
                            default:
                                targetClip = "idle";
                                break;
                        }
                    }

                    animator->setInPlaceLocomotion(targetClip == "walk");

                    // Only switch if the target clip exists and isn't already playing
                    if (animator->hasClip(targetClip) && animator->currentClipName() != targetClip) {
                        animator->play(targetClip, loop, playbackSpeed);
                    }
                }

                if(enemy->currentState == EnemyComponent::State::DEAD) {
                    auto* rbComp = entity->getComponent<RigidBodyComponent>();
                    if (physicsSystem && rbComp && rbComp->bodyHandle) {
                        glm::vec3 velocity = physicsSystem->getPhysicsWorld().getLinearVelocity(entity);
                        velocity.x = 0.0f;
                        velocity.z = 0.0f;
                        physicsSystem->getPhysicsWorld().setLinearVelocity(entity, velocity);
                    }

                    enemy->deathTimer += deltaTime;
                    if(enemy->deathTimer > enemy->deathAnimationDuration) {
                        world->markForRemoval(entity);
                    }
                    continue;
                }

                // Handle Idle state (stun/rest after attack)
                if(enemy->idleTimer > 0.0f) {
                    enemy->idleTimer -= deltaTime;
                    // While idle, stay still but still check if player is around to face them?
                    // For now, just stay completely still.
                    continue;
                }

                glm::vec3 enemyPos = entity->localTransform.position;
                float distToPlayer = playerEntity ? glm::distance(enemyPos, playerEntity->localTransform.position) : FLT_MAX;
                bool touchingPlayer = false;
                if (physicsSystem) {
                    touchingPlayer = physicsSystem->getPhysicsWorld().hasAnyInteraction(entity, playerEntity, true);
                }

                // State Transitions
                // NOTE: detection and waypoint distances are AI navigation logic,
                // not collision/hit detection.
                if(touchingPlayer || distToPlayer < enemy->attackRange) {
                    enemy->currentState = EnemyComponent::State::ATTACK;
                } else if(distToPlayer < enemy->detectionRange) {
                    enemy->currentState = EnemyComponent::State::CHASE;
                } else {
                    enemy->currentState = EnemyComponent::State::PATROL;
                }

                // AI Logic based on current state
                if(enemy->currentState == EnemyComponent::State::PATROL) {
                    if(enemy->waypoints.size() > 1) {
                        glm::vec3 target = enemy->waypoints[enemy->currentWaypointIndex];
                        if(glm::distance(enemyPos, target) < 0.5f) {
                            enemy->currentWaypointIndex = (enemy->currentWaypointIndex + 1) % enemy->waypoints.size();
                        }
                        glm::vec3 moveDir = target - enemyPos;
                        if (glm::length(moveDir) > 0.0001f) {
                            moveDir = glm::normalize(moveDir);
                        } else {
                            moveDir = glm::vec3(0.0f);
                        }

                        auto* rbComp = entity->getComponent<RigidBodyComponent>();
                        if (physicsSystem && rbComp && rbComp->bodyHandle) {
                            glm::vec3 velocity = physicsSystem->getPhysicsWorld().getLinearVelocity(entity);
                            velocity.x = moveDir.x * enemy->patrolSpeed;
                            velocity.z = moveDir.z * enemy->patrolSpeed;
                            physicsSystem->getPhysicsWorld().setLinearVelocity(entity, velocity);
                        } else {
                            entity->localTransform.position += moveDir * enemy->patrolSpeed * deltaTime;
                        }

                        // Rotate to face movement
                        if (glm::length(moveDir) > 0.0001f) {
                            entity->localTransform.rotation.y = glm::atan(moveDir.x, moveDir.z);
                        }
                    }
                } 
                else if(enemy->currentState == EnemyComponent::State::CHASE && playerEntity) {
                    glm::vec3 moveDir = playerEntity->localTransform.position - enemyPos;
                    if (glm::length(moveDir) > 0.0001f) {
                        moveDir = glm::normalize(moveDir);
                    } else {
                        moveDir = glm::vec3(0.0f);
                    }

                    auto* rbComp = entity->getComponent<RigidBodyComponent>();
                    if (physicsSystem && rbComp && rbComp->bodyHandle) {
                        glm::vec3 velocity = physicsSystem->getPhysicsWorld().getLinearVelocity(entity);
                        velocity.x = moveDir.x * enemy->chaseSpeed;
                        velocity.z = moveDir.z * enemy->chaseSpeed;
                        physicsSystem->getPhysicsWorld().setLinearVelocity(entity, velocity);
                    } else {
                        entity->localTransform.position += moveDir * enemy->chaseSpeed * deltaTime;
                    }

                    if (glm::length(moveDir) > 0.0001f) {
                        entity->localTransform.rotation.y = glm::atan(moveDir.x, moveDir.z);
                    }
                } 
                else if(enemy->currentState == EnemyComponent::State::ATTACK && playerEntity) {
                    auto* rbComp = entity->getComponent<RigidBodyComponent>();
                    if (physicsSystem && rbComp && rbComp->bodyHandle) {
                        glm::vec3 velocity = physicsSystem->getPhysicsWorld().getLinearVelocity(entity);
                        velocity.x = 0.0f;
                        velocity.z = 0.0f;
                        physicsSystem->getPhysicsWorld().setLinearVelocity(entity, velocity);
                    }

                    enemy->currentAttackTimer -= deltaTime;
                    if(enemy->currentAttackTimer <= 0.0f) {
                        // Perform Attack
                        if(playerController->invincibilityTimer <= 0.0f) {
                            playerController->health -= enemy->damage;
                            playerController->invincibilityTimer = playerController->invincibilityDuration;
                            std::cout << "[EnemySystem] Player attacked by " << entity->name << "! Health: " << playerController->health << std::endl;
                            
                            if(playerController->health <= 0) {
                                playerController->lives--;
                                std::cout << "[EnemySystem] Player died! Lives remaining: " << playerController->lives << std::endl;

                                if(playerController->lives > 0){
                                    playerController->health = 100;
                                    playerEntity->localTransform.position = playerController->respawnPosition;

                                    if (auto* rbComp = playerEntity->getComponent<RigidBodyComponent>(); rbComp && rbComp->bodyHandle) {
                                        reactphysics3d::Transform respawnTransform = rbComp->bodyHandle->getTransform();
                                        respawnTransform.setPosition(reactphysics3d::Vector3(
                                            playerController->respawnPosition.x,
                                            playerController->respawnPosition.y,
                                            playerController->respawnPosition.z
                                        ));
                                        rbComp->bodyHandle->setTransform(respawnTransform);
                                        rbComp->bodyHandle->setLinearVelocity(reactphysics3d::Vector3(0, 0, 0));
                                        rbComp->velocity = glm::vec3(0.0f);
                                    }

                                    AudioSystem::instance().playSound("assets/audio/afterDeath.wav");
                                    std::cout << "[EnemySystem] Respawning at " << playerController->respawnPosition.x << ", " << playerController->respawnPosition.y << ", " << playerController->respawnPosition.z << std::endl;
                                } else {
                                    AudioSystem::instance().playSound("assets/audio/death.wav");
                                    std::cout << "[EnemySystem] GAME OVER! No more lives." << std::endl;
                                }
                            } else {
                                // Non-lethal hit — play hit sound once
                                AudioSystem::instance().playSound("assets/audio/hit.wav");
                            }
                        }
                        enemy->currentAttackTimer = enemy->attackCooldown;
                        // After an attack, enter idle state to give the player a chance to move or hit back
                        enemy->idleTimer = enemy->idleAfterAttackDuration;
                    }
                    // Face player while attacking
                    glm::vec3 lookDir = playerEntity->localTransform.position - enemyPos;
                    if (glm::length(lookDir) > 0.0001f) {
                        lookDir = glm::normalize(lookDir);
                        entity->localTransform.rotation.y = glm::atan(lookDir.x, lookDir.z);
                    }
                }
            }
        }

        void onImmediateGui(World* world) {
            Entity* cameraEntity = nullptr;
            CameraComponent* camera = nullptr;
            for(auto entity : world->getEntities()){
                camera = entity->getComponent<CameraComponent>();
                if(camera) {
                    cameraEntity = entity;
                    break;
                }
            }
            if(!cameraEntity || !camera) return;

            glm::mat4 V = camera->getViewMatrix();
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
            glm::mat4 P = camera->getProjectionMatrix(glm::ivec2((int)screenSize.x, (int)screenSize.y));
            glm::mat4 VP = P * V;

            for(auto entity : world->getEntities()){
                EnemyComponent* enemy = entity->getComponent<EnemyComponent>();
                if(!enemy || enemy->healthBarTimer <= 0.0f || enemy->currentState == EnemyComponent::State::DEAD) continue;

                // Position above enemy head
                glm::vec3 worldPos = entity->localTransform.position + glm::vec3(0, 2.5f, 0);
                glm::vec4 clipPos = VP * glm::vec4(worldPos, 1.0f);

                if(clipPos.w <= 0.0f) continue; // Behind camera

                glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
                if(ndcPos.x < -1.0f || ndcPos.x > 1.0f || ndcPos.y < -1.0f || ndcPos.y > 1.0f) continue;

                ImVec2 screenSize = ImGui::GetIO().DisplaySize;
                ImVec2 screenPos = ImVec2(
                    (ndcPos.x + 1.0f) * 0.5f * screenSize.x,
                    (1.0f - ndcPos.y) * 0.5f * screenSize.y // Invert Y for screen space
                );

                // Render small floating health bar
                float width = 60.0f;
                float height = 6.0f;
                ImDrawList* drawList = ImGui::GetForegroundDrawList();
                
                ImVec2 p1 = ImVec2(screenPos.x - width*0.5f, screenPos.y - height*0.5f);
                ImVec2 p2 = ImVec2(screenPos.x + width*0.5f, screenPos.y + height*0.5f);
                
                // Background (dark)
                drawList->AddRectFilled(p1, p2, IM_COL32(0, 0, 0, 180));
                
                // Foreground (Health)
                float healthFrac = glm::clamp((float)enemy->health / (float)enemy->maxHealth, 0.0f, 1.0f);
                ImVec2 p2_health = ImVec2(p1.x + width * healthFrac, p2.y);
                
                ImU32 healthColor = IM_COL32(255, 0, 0, 255); // Red for enemies
                if (healthFrac > 0.5f) healthColor = IM_COL32(255, 200, 0, 255); // Orange/Yellow
                
                drawList->AddRectFilled(p1, p2_health, healthColor);
                
                // Border
                drawList->AddRect(p1, p2, IM_COL32(255, 255, 255, 200));
            }
        }
    };

}
