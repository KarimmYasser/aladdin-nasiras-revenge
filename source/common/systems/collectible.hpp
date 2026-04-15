#pragma once

#include "../ecs/world.hpp"
#include "../components/collectible.hpp"
#include "../components/aladdin-controller.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace our {

    /**
     * @brief System responsible for managing collectible items.
     * 
     * It handles:
     * 1. Visual animation (spinning and bobbing).
     * 2. Proximity-based collection detection.
     * 3. Updating Aladdin's inventory and removing the collectible entity.
     */
    class CollectibleSystem {
    public:

        /**
         * @brief Updates all collectibles in the world.
         * 
         * @param world The world containing entities and components.
         * @param deltaTime The time elapsed since the last frame.
         */
        void update(World* world, float deltaTime) {
            
            // Find Aladdin's entity to check for proximity
            Entity* aladdinEntity = nullptr;
            AladdinControllerComponent* aladdin = nullptr;

            // Search for the entity with the Aladdin Controller component
            for(auto entity : world->getEntities()){
                aladdin = entity->getComponent<AladdinControllerComponent>();
                if(aladdin){
                    aladdinEntity = entity;
                    break;
                }
            }

            // Loop through all entities and process those with a CollectibleComponent
            for(auto entity : world->getEntities()){
                CollectibleComponent* collectible = entity->getComponent<CollectibleComponent>();
                if(!collectible) continue;

                // 1. Visual Animation: Handle Rotation and Bobbing
                
                // Rotation
                entity->localTransform.rotation.y += collectible->rotationSpeed * deltaTime;

                // Capture initial Y position once to bob around it
                if(!collectible->initialYSet){
                    collectible->initialY = entity->localTransform.position.y;
                    collectible->initialYSet = true;
                }

                // Bobbing (sine wave motion)
                collectible->animationTimer += deltaTime;
                float verticalOffset = glm::sin(collectible->animationTimer * collectible->bobbingFrequency) * collectible->bobbingHeight;
                entity->localTransform.position.y = collectible->initialY + verticalOffset;

                // 2. Collection Logic: Distance check
                // TODO (Physics): Once Member 2 implements the PhysicsSystem, replace this 
                // distance check with a Trigger Collider event.
                if(aladdinEntity){
                    float distance = glm::distance(aladdinEntity->localTransform.position, entity->localTransform.position);
                    
                    if(distance < collectible->collectionRadius){
                        
                        // Increment Aladdin's stats based on the collectible type
                        switch(collectible->type){
                            case CollectibleComponent::Type::COIN:
                                aladdin->coinCount += collectible->value;
                                std::cout << "[CollectibleSystem] Collected Coin! (Value: " << collectible->value 
                                        << "). Total Coins: " << aladdin->coinCount << std::endl;
                                break;
                            case CollectibleComponent::Type::GEM:
                                aladdin->gemCount += collectible->value;
                                std::cout << "[CollectibleSystem] Collected Gem! (Value: " << collectible->value 
                                        << "). Total Gems: " << aladdin->gemCount << std::endl;
                                break;
                            case CollectibleComponent::Type::APPLE:
                                aladdin->appleCount += collectible->value;
                                std::cout << "[CollectibleSystem] Collected Apple! (Value: " << collectible->value 
                                        << "). Total Apples: " << aladdin->appleCount << std::endl;
                                break;
                        }

                        // Mark the collectible for removal from the world
                        world->markForRemoval(entity);
                    }
                }
            }
        }
    };

}
