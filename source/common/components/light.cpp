#include "light.hpp"
#include <glm/glm.hpp>

namespace our {
    // same arch like the camera component
    void LightComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        std::string typeStr = data.value("light_type", "directional");
        if      (typeStr == "point") type = LightType::POINT;
        else if (typeStr == "spot")  type = LightType::SPOT;
        else                         type = LightType::DIRECTIONAL;

        if (data.contains("color"))
            color = data["color"].get<glm::vec3>();
        intensity = data.value("intensity", 1.0f);

        if (data.contains("attenuation")) {
            auto& att = data["attenuation"];
            attenuation_constant  = att.value("constant",  1.0f);
            attenuation_linear    = att.value("linear",    0.09f);
            attenuation_quadratic = att.value("quadratic", 0.032f);
        }

        inner_angle = data.value("inner_angle", 12.5f);
        outer_angle = data.value("outer_angle", 17.5f);
    }

}