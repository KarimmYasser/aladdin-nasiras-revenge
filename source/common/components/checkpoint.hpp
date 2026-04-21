#pragma once

#include "ecs/component.hpp"
#include <glm/glm.hpp>

namespace our {

    // This component marks an entity as a checkpoint.
    // When the player gets close enough, it updates the player's respawn position.
    struct CheckpointComponent : public Component {
        float radius = 2.0f;        // Activation radius
        bool activated = false;     // Has this checkpoint been reached?
        
        // The color to change to when activated (visual feedback)
        glm::vec3 activeColor = {0.0f, 1.0f, 0.0f}; // Green

        // The ID of the component type for deserialization
        static std::string getID() { return "Checkpoint"; }

        // Deserialize the component data from a JSON object
        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            radius = data.value("radius", radius);
            if(data.contains("activeColor")){
                activeColor = {data["activeColor"][0], data["activeColor"][1], data["activeColor"][2]};
            }
        }
    };

}