#pragma once

#include "../ecs/world.hpp"
#include "../components/enemy.hpp"
#include "../components/aladdin-controller.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace our {

    class EnemySystem {
    public:
        void update(World* world, float deltaTime) {
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

                // State Transitions
                if(distToPlayer < enemy->attackRange) {
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
                        glm::vec3 moveDir = glm::normalize(target - enemyPos);
                        entity->localTransform.position += moveDir * enemy->patrolSpeed * deltaTime;
                        // Rotate to face movement
                        entity->localTransform.rotation.y = glm::atan(moveDir.x, moveDir.z);
                    }
                } 
                else if(enemy->currentState == EnemyComponent::State::CHASE && playerEntity) {
                    glm::vec3 moveDir = glm::normalize(playerEntity->localTransform.position - enemyPos);
                    entity->localTransform.position += moveDir * enemy->chaseSpeed * deltaTime;
                    entity->localTransform.rotation.y = glm::atan(moveDir.x, moveDir.z);
                } 
                else if(enemy->currentState == EnemyComponent::State::ATTACK && playerEntity) {
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
                    glm::vec3 lookDir = glm::normalize(playerEntity->localTransform.position - enemyPos);
                    entity->localTransform.rotation.y = glm::atan(lookDir.x, lookDir.z);
                }
            }
        }
    };

}
