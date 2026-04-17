#pragma once

#include "../ecs/world.hpp"
#include "../components/hazard.hpp"
#include "../components/aladdin-controller.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace our {

    /**
     * @brief System that handles interactions between hazards and the player.
     * 
     * Since real physics is postponed, it uses distance-based detection.
     */
    class HazardSystem {
    public:
        /**
         * @brief Checks if the player is within any hazard's radius and applies damage.
         * 
         * @param world The scene world containing entities.
         * @param deltaTime The time elapsed since the last frame.
         */
        void update(World* world, float deltaTime) {
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

            if(!playerEntity || !playerController) return;

            // 2. Iterate through all hazards
            for(auto entity : world->getEntities()){
                HazardComponent* hazard = entity->getComponent<HazardComponent>();
                if(!hazard) continue;

                // 3. Mock Collision Detection: Check distance between player and hazard
                float distance = glm::distance(playerEntity->localTransform.position, entity->localTransform.position);
                
                // 4. If colliding and player is not invincible
                if(distance < hazard->radius && playerController->invincibilityTimer <= 0.0f){
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
                            std::cout << "[HazardSystem] Respawning at " << playerController->respawnPosition.x << ", " << playerController->respawnPosition.y << ", " << playerController->respawnPosition.z << std::endl;
                        } else {
                            // Game Over
                            std::cout << "[HazardSystem] GAME OVER! No more lives." << std::endl;
                            // TODO: Trigger transition to GameOverState or reset level
                        }
                    } else {
                        // Just a hit, trigger invincibility
                        playerController->invincibilityTimer = playerController->invincibilityDuration;
                    }
                }
            }
        }
    };

}
