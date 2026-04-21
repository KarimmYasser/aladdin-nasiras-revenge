#pragma once

#include "pipeline-state.hpp"
#include "../texture/texture2d.hpp"
#include "../texture/sampler.hpp"
#include "../shader/shader.hpp"
#include "material.hpp"

#include <glm/vec3.hpp>
#include <json/json.hpp>

namespace our {
    // LitMaterial is a material that supports the full Blinn-Phong lighting model.
    // It uses the "light.vert" / "light.frag" shader pair and supports the following
    // texture maps that control how the surface interacts with light:
    //   - albedo_map   : base color (like a diffuse texture)
    //   - specular_map : per-pixel specular intensity (white = full spec, black = no spec)
    //   - emission_map : self-illumination (surfaces that glow regardless of lights)
    class LitMaterial : public Material {
    public:
        // all optional — fallback to white if nullptr
        Texture2D* albedo_map   = nullptr;
        Texture2D* specular_map = nullptr;
        Texture2D* emission_map = nullptr;

        // Shared sampler for all texture maps
        Sampler* sampler = nullptr;

        glm::vec2 uv_multiplier = {1.0f, 1.0f};

        glm::vec3 albedo_tint = {1.0f, 1.0f, 1.0f};

        // Higher = smaller, sharper highlight (metal-like); lower = wider (plastic-like).
        float shininess = 32.0f;

        // a small constant added so shadowed areas aren't pitch black
        float ambient = 0.1f;

        void setup() const override;
        void deserialize(const nlohmann::json& data) override;
    };

}