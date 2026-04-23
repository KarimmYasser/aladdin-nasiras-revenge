#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <json/json.hpp>

namespace our {

    /// teleports the player to another room position.
    struct RoomPortalComponent : public Component {
        float radius = 2.5f;
        glm::vec3 targetPosition{0.0f};
        /// If false, keep the player's current yaw after teleport.
        bool setYaw = true;
        float targetYawDegrees = 0.0f;

        float blackoutDuration = 2.0f;
        float cooldownDuration = 2.0f;
        float cooldownTimer = 0.0f;

        static std::string getID() { return "Room Portal"; }

        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            radius = data.value("radius", radius);
            blackoutDuration = data.value("blackoutDuration", blackoutDuration);
            cooldownDuration = data.value("cooldownDuration", cooldownDuration);
            setYaw = data.value("setYaw", setYaw);
            targetYawDegrees = data.value("targetYaw", targetYawDegrees);
            if(data.contains("targetPosition") && data["targetPosition"].is_array() && data["targetPosition"].size() >= 3){
                const auto& p = data["targetPosition"];
                targetPosition = {p[0].get<float>(), p[1].get<float>(), p[2].get<float>()};
            }
        }
    };

}
