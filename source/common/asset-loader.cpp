#include "asset-loader.hpp"

#include "shader/shader.hpp"
#include "texture/texture2d.hpp"
#include "texture/texture-utils.hpp"
#include "texture/sampler.hpp"
#include "mesh/mesh.hpp"
#include "mesh/mesh-utils.hpp"
#include "material/material.hpp"
#include "deserialize-utils.hpp"
#include <unordered_set>
#include <type_traits>

namespace our {

    // This will load a single shader defined by "desc"
    template<>
    void AssetLoader<ShaderProgram>::deserializeSingle(const std::string& name, const nlohmann::json& desc) {
        if(has(name)) return;
        std::string vsPath = desc.value("vs", "");
        std::string fsPath = desc.value("fs", "");
        auto shader = new ShaderProgram();
        shader->attach(vsPath, GL_VERTEX_SHADER);
        shader->attach(fsPath, GL_FRAGMENT_SHADER);
        shader->link();
        assets[name] = shader;
    }

    // This will load all the shaders defined in "data"
    template<>
    void AssetLoader<ShaderProgram>::deserialize(const nlohmann::json& data) {
        if(data.is_object()){
            for(auto& [name, desc] : data.items()){
                deserializeSingle(name, desc);
            }
        }
    };

    // This will load a single texture
    template<>
    void AssetLoader<Texture2D>::deserializeSingle(const std::string& name, const nlohmann::json& desc) {
        if(has(name)) return;
        std::string path = desc.get<std::string>();
        assets[name] = texture_utils::loadImage(path);
    }

    // This will load all the textures defined in "data"
    template<>
    void AssetLoader<Texture2D>::deserialize(const nlohmann::json& data) {
        if(data.is_object()){
            for(auto& [name, desc] : data.items()){
                deserializeSingle(name, desc);
            }
        }
    };

    // This will load a single sampler
    template<>
    void AssetLoader<Sampler>::deserializeSingle(const std::string& name, const nlohmann::json& desc) {
        if(has(name)) return;
        auto sampler = new Sampler();
        sampler->deserialize(desc);
        assets[name] = sampler;
    }

    // This will load all the samplers defined in "data"
    template<>
    void AssetLoader<Sampler>::deserialize(const nlohmann::json& data) {
        if(data.is_object()){
            for(auto& [name, desc] : data.items()){
                deserializeSingle(name, desc);
            }
        }
    };

    // This will load a single mesh
    template<>
    void AssetLoader<Mesh>::deserializeSingle(const std::string& name, const nlohmann::json& desc) {
        if(has(name)) return;
        std::string path = desc.get<std::string>();
        assets[name] = mesh_utils::loadOBJ(path);
    }

    // This will load all the meshes defined in "data"
    template<>
    void AssetLoader<Mesh>::deserialize(const nlohmann::json& data) {
        if(data.is_object()){
            for(auto& [name, desc] : data.items()){
                deserializeSingle(name, desc);
            }
        }
    };

    // This will load a single material
    template<>
    void AssetLoader<Material>::deserializeSingle(const std::string& name, const nlohmann::json& desc) {
        if(has(name)) return;
        std::string type = desc.value("type", "");
        auto material = createMaterialFromType(type);
        material->deserialize(desc);
        assets[name] = material;
    }

    // This will load all the materials defined in "data"
    template<>
    void AssetLoader<Material>::deserialize(const nlohmann::json& data) {
        if(data.is_object()){
            for(auto& [name, desc] : data.items()){
                deserializeSingle(name, desc);
            }
        }
    };

    void deserializeAllAssets(const nlohmann::json& assetData){
        if(!assetData.is_object()) return;
        if(assetData.contains("shaders"))
            AssetLoader<ShaderProgram>::deserialize(assetData["shaders"]);
        if(assetData.contains("textures"))
            AssetLoader<Texture2D>::deserialize(assetData["textures"]);
        if(assetData.contains("samplers"))
            AssetLoader<Sampler>::deserialize(assetData["samplers"]);
        if(assetData.contains("meshes"))
            AssetLoader<Mesh>::deserialize(assetData["meshes"]);
        if(assetData.contains("materials"))
            AssetLoader<Material>::deserialize(assetData["materials"]);
    }

    void clearAllAssets(){
        AssetLoader<ShaderProgram>::clear();
        AssetLoader<Texture2D>::clear();
        AssetLoader<Sampler>::clear();
        AssetLoader<Mesh>::clear();
        AssetLoader<Material>::clear();
    }

    void evictUnusedAssets(const nlohmann::json& neededAssets){
        if(!neededAssets.is_object()) return;

        auto cleanup = [&](auto* type_ptr, const std::string& key){
            using T = std::remove_pointer_t<decltype(type_ptr)>;
            std::unordered_set<std::string> needed;
            if(neededAssets.contains(key) && neededAssets[key].is_object()){
                for(auto& [name, _] : neededAssets[key].items()) needed.insert(name);
            }
            for(const auto& name : AssetLoader<T>::getKeys()){
                if(needed.find(name) == needed.end()) AssetLoader<T>::evict(name);
            }
        };

        cleanup((ShaderProgram*)nullptr, "shaders");
        cleanup((Texture2D*)nullptr, "textures");
        cleanup((Sampler*)nullptr, "samplers");
        cleanup((Mesh*)nullptr, "meshes");
        cleanup((Material*)nullptr, "materials");
    }

}