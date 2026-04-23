#pragma once

#include "../ecs/world.hpp"
#include "../components/collectible.hpp"
#include "../components/aladdin-controller.hpp"
#include "../physics/physics-system.hpp"
#include "../audio/audio-system.hpp"
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
        void update(World* world, const PhysicsSystem* physicsSystem, float deltaTime) {
            
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

                // 2. Collection Logic: physics overlap when present, else distance (coins often have no RigidBody).
                if(aladdinEntity){
                    bool collected = false;
                    if (physicsSystem) {
                        const auto& physicsWorld = physicsSystem->getPhysicsWorld();
                        collected = physicsWorld.hasTriggerEvent(aladdinEntity, entity, true) ||
                                    physicsWorld.hasContactEvent(aladdinEntity, entity, true);
                    }
                    if (!collected) {
                        const glm::vec3 d = aladdinEntity->localTransform.position - entity->localTransform.position;
                        const glm::vec3& sc = entity->localTransform.scale;
                        const float scaleMax = glm::max(glm::max(sc.x, sc.y), sc.z);
                        const float scaleBoost = glm::max(0.0f, 0.38f * scaleMax);
                        const float reach = collectible->collectionRadius + scaleBoost + 0.55f;
                        collected = glm::dot(d, d) <= reach * reach;
                    }

                    if(collected){
                        
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
                            case CollectibleComponent::Type::KEY:
                                aladdin->hasKey = true;
                                std::cout << "[CollectibleSystem] Collected Key! Level Exit is now active." << std::endl;
                                break;
                        }

                        // Play collection sound effect
                        AudioSystem::instance().playSound("assets/audio/coinCollected.wav");

                        // Mark the collectible for removal from the world
                        world->markForRemoval(entity);
                    }
                }
            }
        }
    };

}
