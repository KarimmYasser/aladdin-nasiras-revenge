#pragma once

#include "../ecs/world.hpp"
#include "../components/hazard.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/rigid-body.hpp"
#include "../physics/physics-system.hpp"
#include "../audio/audio-system.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace our {

    /**
     * @brief System that handles interactions between hazards and the player.
     * 
     * Uses physics trigger/contact events to detect hazard hits.
     */
    class HazardSystem {
    public:
        /**
         * @brief Checks if the player is within any hazard's radius and applies damage.
         * 
         * @param world The scene world containing entities.
         * @param physicsSystem Physics system used to query overlap/contact events.
         * @param deltaTime The time elapsed since the last frame.
         */
        void update(World* world, PhysicsSystem* physicsSystem, float /*deltaTime*/) {
            // 1. Find the player (entity with AladdinControllerComponent)
            Entity* playerEntity = nullptr;
            AladdinControllerComponent* playerController = nullptr;
            for(auto entity : world->getEntities()){
                playerController = entity->getComponent<AladdinControllerComponent>();
                if(playerController){
                    playerEntity = entity;
                    break;
                }
            }

            if(!playerEntity || !playerController || playerController->lives <= 0) return;

            // 2. Iterate through all hazards
            for(auto entity : world->getEntities()){
                HazardComponent* hazard = entity->getComponent<HazardComponent>();
                if(!hazard) continue;

                if(!physicsSystem) continue;

                const auto& physicsWorld = physicsSystem->getPhysicsWorld();
                const bool isColliding = physicsWorld.hasTriggerEvent(playerEntity, entity) ||
                                         physicsWorld.hasContactEvent(playerEntity, entity);

                // 4. If colliding and player is not invincible
                if(isColliding && playerController->invincibilityTimer <= 0.0f){
                    // Apply Damage
                    if(hazard->instakill){
                        playerController->health = 0;
                    } else {
                        playerController->health -= hazard->damage;
                    }
                    
                    std::cout << "[HazardSystem] Player hit by " << entity->name << "! Health: " << playerController->health << std::endl;

                    // 5. Handle Player Death and Lives
                    if(playerController->health <= 0){
                        playerController->lives--;
                        std::cout << "[HazardSystem] Player died! Lives remaining: " << playerController->lives << std::endl;

                        if(playerController->lives > 0){
                            // Respawn at checkpoint
                            playerEntity->localTransform.position = playerController->respawnPosition;
                            playerController->health = 100; // Reset health to 100
                            playerController->velocity = {0,0,0};
                            playerController->invincibilityTimer = playerController->invincibilityDuration;

                            // Keep rigidbody state in sync with respawn
                            if (auto* rbComp = playerEntity->getComponent<RigidBodyComponent>(); rbComp && rbComp->bodyHandle) {
                                reactphysics3d::Transform respawnTransform = rbComp->bodyHandle->getTransform();
                                respawnTransform.setPosition(reactphysics3d::Vector3(
                                    playerController->respawnPosition.x,
                                    playerController->respawnPosition.y,
                                    playerController->respawnPosition.z
                                ));
                                rbComp->bodyHandle->setTransform(respawnTransform);
                                rbComp->bodyHandle->setLinearVelocity(reactphysics3d::Vector3(0, 0, 0));
                            }

                            AudioSystem::instance().playSound("assets/audio/afterDeath.wav");
                            std::cout << "[HazardSystem] Respawning at " << playerController->respawnPosition.x << ", " << playerController->respawnPosition.y << ", " << playerController->respawnPosition.z << std::endl;
                        } else {
                            // Game Over
                            AudioSystem::instance().playSound("assets/audio/death.wav");
                            std::cout << "[HazardSystem] GAME OVER! No more lives." << std::endl;
                            // TODO: Trigger transition to GameOverState or reset level
                        }
                    } else {
                        // Just a hit, trigger invincibility
                        AudioSystem::instance().playSound("assets/audio/hit.wav");
                        playerController->invincibilityTimer = playerController->invincibilityDuration;
                    }
                }
            }
        }
    };

}
