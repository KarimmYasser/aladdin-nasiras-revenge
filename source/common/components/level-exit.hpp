#pragma once

#include "ecs/component.hpp"
#include <string>

namespace our {

    // This component marks an entity as a level exit.
    // It requires the player to have a "Key" to activate.
    struct LevelExitComponent : public Component {
        float radius = 2.5f;                // Activation radius
        std::string nextScene = "";         // The JSON config for the next level
        bool activated = false;             // Has the exit been successfully triggered?
        
        // The ID of the component type for deserialization
        static std::string getID() { return "LevelExit"; }

        // Deserialize the component data from a JSON object
        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            radius = data.value("radius", radius);
            nextScene = data.value("nextScene", nextScene);
        }
    };

}
