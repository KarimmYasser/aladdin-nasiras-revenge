#include "mesh-renderer.hpp"
#include "../asset-loader.hpp"

namespace our {
    // Receives the mesh & material from the AssetLoader by the names given in the json object
    void MeshRendererComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        // Notice how we just get a string from the json file and pass it to the AssetLoader to get us the actual asset
        //TODO: (Req 8) Get the material and the mesh from the AssetLoader by their names
        // which are defined with the keys "mesh" and "material" in data.
        mesh     = AssetLoader<Mesh>::get(data["mesh"].get<std::string>());
        material = AssetLoader<Material>::get(data["material"].get<std::string>());
        visible = data.value("visible", visible);
    }
}