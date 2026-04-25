#pragma once

#include <string>
#include <unordered_map>

#include "../ecs/component.hpp"
#include "../mesh/mesh.hpp"
#include "../material/material.hpp"
#include "../asset-loader.hpp"

namespace our {

    class MeshRendererComponent : public Component {
    public:
        Mesh* mesh = nullptr;
        /// Default / fallback material when no per-OBJ-material mapping exists.
        Material* material = nullptr;
        /// Maps tinyobj material name (e.g. mat0) → LitMaterial from scene assets.
        std::unordered_map<std::string, Material*> submeshMaterials;

        bool visible = true;

        static std::string getID() { return "Mesh Renderer"; }

        Material* resolveMaterialForSubmesh(const std::string& objMaterialName) const {
            auto it = submeshMaterials.find(objMaterialName);
            if (it != submeshMaterials.end() && it->second) return it->second;
            return material;
        }

        void deserialize(const nlohmann::json& data) override;
    };

}