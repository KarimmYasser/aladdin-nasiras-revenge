#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <string>

namespace our {

    /**
     * @brief This component defines a collectible item in the game (Coins, Gems, Apples).
     * 
     * It holds data for the type of collectible, its value, and parameters for its
     * visual animation (spinning and bobbing).
     */
    class CollectibleComponent : public Component {
    public:
        // The type of collectible (determines which counter in AladdinController to increment)
        enum class Type {
            COIN,
            GEM,
            APPLE
        };

        Type type = Type::COIN;           // Default type is Coin
        int value = 1;                    // How many to add when collected
        float collectionRadius = 1.5f;    // Distance threshold for collection (Mock Physics)

        // Visual Animation settings
        float rotationSpeed = 2.0f;       // Radians per second
        float bobbingHeight = 0.5f;       // Max vertical displacement
        float bobbingFrequency = 2.0f;    // Speed of the bobbing motion

        // Runtime state for animation
        float animationTimer = 0.0f;      // Accumulated time for sine wave
        float initialY = 0.0f;            // Stores the starting Y position to bob around
        bool initialYSet = false;         // Helper to capture the initial position once

        // The ID of this component type is "Collectible"
        static std::string getID() { return "Collectible"; }

        /**
         * @brief Reads collectible properties from a JSON object.
         * 
         * @param data The JSON object containing type, value, radius, etc.
         */
        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            
            std::string typeStr = data.value("collectibleType", "coin");
            if(typeStr == "coin") type = Type::COIN;
            else if(typeStr == "gem") type = Type::GEM;
            else if(typeStr == "apple") type = Type::APPLE;

            value = data.value("value", value);
            collectionRadius = data.value("collectionRadius", collectionRadius);
            rotationSpeed = data.value("rotationSpeed", rotationSpeed);
            bobbingHeight = data.value("bobbingHeight", bobbingHeight);
            bobbingFrequency = data.value("bobbingFrequency", bobbingFrequency);
        }
    };

}
