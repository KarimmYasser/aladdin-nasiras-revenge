#pragma once

#include "../ecs/world.hpp"
#include "../components/level-exit.hpp"
#include "../components/aladdin-controller.hpp"
#include "../physics/physics-system.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace our {

    /**
     * @brief System responsible for managing level transitions.
     * 
     * It checks if Aladdin is near the level exit and if he has the required key.
     */
    class LevelExitSystem {
        std::string nextScene = "";
    public:

        /**
         * @brief Updates level exit checks.
         * 
         * @param world The world containing entities and components.
         */
        void update(World* world, const PhysicsSystem* physicsSystem) {
            
            Entity* aladdinEntity = nullptr;
            AladdinControllerComponent* aladdin = nullptr;

            // Find Aladdin
            for(auto entity : world->getEntities()){
                aladdin = entity->getComponent<AladdinControllerComponent>();
                if(aladdin){
                    aladdinEntity = entity;
                    break;
                }
            }

            if(!aladdinEntity) return;

            // Find Level Exit entities
            for(auto entity : world->getEntities()){
                LevelExitComponent* exit = entity->getComponent<LevelExitComponent>();
                if(!exit || exit->activated) continue;

                bool isAtExit = false;
                if (physicsSystem) {
                    const auto& physicsWorld = physicsSystem->getPhysicsWorld();
                    isAtExit = physicsWorld.hasTriggerEvent(aladdinEntity, entity) ||
                               physicsWorld.hasContactEvent(aladdinEntity, entity);
                }

                if(isAtExit){
                    if(aladdin->hasKey){
                        exit->activated = true;
                        std::cout << "[LevelExitSystem] Level Complete! Moving to: " << exit->nextScene << std::endl;
                        nextScene = exit->nextScene;
                    } else {
                        // Optional: Show a hint that a key is needed
                        static float hintTimer = 0.0f;
                        hintTimer -= 0.016f; // Rough estimation of frame time
                        if(hintTimer <= 0.0f){
                            std::cout << "[LevelExitSystem] The door is locked. Find the KEY first!" << std::endl;
                            hintTimer = 2.0f; // Show every 2 seconds
                        }
                    }
                }
            }
        }

        std::string getNextScene() { return nextScene; }
        void clearNextScene() { nextScene = ""; }
    };

}
