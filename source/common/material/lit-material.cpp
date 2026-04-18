#include "material.hpp"
#include "../asset-loader.hpp"
#include "../texture/texture-utils.hpp"
#include "deserialize-utils.hpp"

#include <glad/gl.h>

namespace our {
    //   Unit 0 -> albedo_map   ("albedo_map"   uniform)
    //   Unit 1 -> specular_map ("specular_map" uniform)
    //   Unit 2 -> emission_map ("emission_map" uniform)
    void LitMaterial::setup() const {
        pipelineState.setup();
        shader->use();

        shader->set("material.shininess",   shininess);
        shader->set("material.ambient",     ambient);
        shader->set("material.albedo_tint", albedo_tint);

        // We create a static white texture once and reuse it for missing maps.
        static Texture2D* white = texture_utils::singleColor({255, 255, 255, 255});

        // --- Albedo (unit 0) ---
        glActiveTexture(GL_TEXTURE0);
        (albedo_map ? albedo_map : white)->bind();
        if (sampler) sampler->bind(0);
        shader->set("material.albedo_map", (GLint)0);

        // --- Specular (unit 1) ---
        glActiveTexture(GL_TEXTURE1);
        (specular_map ? specular_map : white)->bind();
        if (sampler) sampler->bind(1);
        shader->set("material.specular_map", (GLint)1);

        // --- Emission (unit 2) ---
        glActiveTexture(GL_TEXTURE2);
        (emission_map ? emission_map : white)->bind();
        if (sampler) sampler->bind(2);
        shader->set("material.emission_map", (GLint)2);
    }

    // deserialize() reads LitMaterial parameters from a JSON object.
    // Example JSON:
    // {
    //   "type": "lit",
    //   "shader": "light",
    //   "albedo_map": "moon",
    //   "shininess": 64.0,
    //   "albedo_tint": [1, 0.9, 0.8]
    // }
    void LitMaterial::deserialize(const nlohmann::json& data) {
        Material::deserialize(data);
        if (!data.is_object()) return;

        if (data.contains("albedo_tint"))
            albedo_tint = data["albedo_tint"].get<glm::vec3>();

        shininess = data.value("shininess", 32.0f);
        ambient   = data.value("ambient",   0.1f);

        // Look up textures from the AssetLoader by name (they were loaded earlier)
        albedo_map   = AssetLoader<Texture2D>::get(data.value("albedo_map",   ""));
        specular_map = AssetLoader<Texture2D>::get(data.value("specular_map", ""));
        emission_map = AssetLoader<Texture2D>::get(data.value("emission_map", ""));
        sampler      = AssetLoader<Sampler>::get(data.value("sampler",        ""));
    }

}
