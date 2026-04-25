#include "mesh-renderer.hpp"

namespace our {

    void MeshRendererComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        mesh = AssetLoader<Mesh>::get(data["mesh"].get<std::string>());
        submeshMaterials.clear();

        if (data.contains("submeshMaterials") && data["submeshMaterials"].is_object()) {
            for (auto& [key, val] : data["submeshMaterials"].items()) {
                if (val.is_string()) {
                    submeshMaterials[key] = AssetLoader<Material>::get(val.get<std::string>());
                }
            }
        }

        if (data.contains("material")) {
            material = AssetLoader<Material>::get(data["material"].get<std::string>());
        } else if (!submeshMaterials.empty()) {
            material = submeshMaterials.begin()->second;
        } else {
            material = nullptr;
        }

        visible = data.value("visible", visible);
    }
}