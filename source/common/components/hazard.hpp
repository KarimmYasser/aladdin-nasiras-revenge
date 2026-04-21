#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our {

    /**
     * @brief This component defines a hazard that can damage the player.
     * 
     * Since physics is not yet integrated, it uses a simple radius-based detection.
     */
    class HazardComponent : public Component {
    public:
        int damage = 10;                // How much health to remove
        float radius = 1.0f;            // Detection radius (Mock Physics)
        bool instakill = false;         // If true, kills player regardless of health

        // The ID of this component type is "Hazard"
        static std::string getID() { return "Hazard"; }

        /**
         * @brief Reads hazard properties from a JSON object.
         * 
         * @param data The JSON object containing "damage", "radius", and "instakill".
         */
        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            damage = data.value("damage", damage);
            radius = data.value("radius", radius);
            instakill = data.value("instakill", instakill);
        }
    };

}
