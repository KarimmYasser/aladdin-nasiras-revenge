#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our {
    class LightComponent : public Component {
    public:
        enum class LightType {
            DIRECTIONAL, // like the sun
            POINT,       // like a lamp
            SPOT         // like a torch
        };

        LightType type = LightType::DIRECTIONAL;

        glm::vec3 color     = {1.0f, 1.0f, 1.0f}; // RGB color of emitted light
        float     intensity = 1.0f;                // Brightness multiplier

        // --- Attenuation (Point & Spot only) ---
        // Real-world falloff: att = 1 / (constant + linear*d + quadratic*d²)
        float attenuation_constant  = 1.0f;
        float attenuation_linear    = 0.09f;
        float attenuation_quadratic = 0.032f;

        // Spot only, stored in degrees, converted on deserialization
        float inner_angle = 12.5f; // Full-intensity cone (degrees)
        float outer_angle = 17.5f; // Fade-to-zero cone  (degrees)

        static std::string getID() { return "Light"; }
        void deserialize(const nlohmann::json& data) override;
    };

}