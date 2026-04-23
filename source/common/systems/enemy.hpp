#pragma once

#include "../ecs/world.hpp"
#include "../components/enemy.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/rigid-body.hpp"
#include "../physics/physics-system.hpp"
#include <glm/glm.hpp>
#include <cfloat>
#include <iostream>

namespace our {

    class EnemySystem {
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

                if(enemy->currentState == EnemyComponent::State::DEAD) {
                    auto* rbComp = entity->getComponent<RigidBodyComponent>();
                    if (physicsSystem && rbComp && rbComp->bodyHandle) {
                        glm::vec3 velocity = physicsSystem->getPhysicsWorld().getLinearVelocity(entity);
                        velocity.x = 0.0f;
                        velocity.z = 0.0f;
                        physicsSystem->getPhysicsWorld().setLinearVelocity(entity, velocity);
                    }

                    enemy->deathTimer += deltaTime;
                    if(enemy->deathTimer > 2.0f) world->markForRemoval(entity);
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
                    if(!enemy->waypoints.empty()) {
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

                                    std::cout << "[EnemySystem] Respawning at " << playerController->respawnPosition.x << ", " << playerController->respawnPosition.y << ", " << playerController->respawnPosition.z << std::endl;
                                } else {
                                    std::cout << "[EnemySystem] GAME OVER! No more lives." << std::endl;
                                }
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
    };

}
