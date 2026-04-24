#pragma once

#include "../ecs/component.hpp"
#include "../animation/animation-loader.hpp"
#include "../animation/animator.hpp"
#include "../mesh/skinned-mesh.hpp"
#include "../material/material.hpp"
#include "../asset-loader.hpp"
#include <iostream>
#include <algorithm>

namespace our {

    // SkinnedMeshRendererComponent
    // ----------------------------
    // A single component for any entity that should display an animated,
    // GPU-skinned character mesh.  It replaces the old two-component pattern
    // of attaching *both* AnimatorComponent + MeshRendererComponent to the
    // same entity.
    //
    // Responsibilities:
    //   - Loads a DAE/FBX model via AnimationLoader (owns the SkinnedMesh)
    //   - Resolves a named Material from AssetLoader (same pattern as MeshRendererComponent)
    //   - Drives the Animator each frame via AnimationSystem::update()
    //   - Exposes `visible` so the first-person camera can hide the mesh
    //
    // JSON example (inside an entity's "components" array):
    // {
    //     "type":       "Skinned Mesh Renderer",
    //     "model":      "assets/models/Aladdin/aladdin_costume_basic.dae",
    //     "animations": ["assets/models/Aladdin/animations/walk.fbx",
    //                    "assets/models/Aladdin/animations/jump.fbx"],
    //     "material":   "lit-aladdin",
    //     "clip":       "walk"
    // }
    class SkinnedMeshRendererComponent : public Component {
    public:
        SkinnedMesh*               skinnedMesh = nullptr;  // owned; deleted in destructor
        std::vector<Material*>     materials;              // resolved from AssetLoader; NOT owned
        std::vector<AnimationClip> clips;
        Animator                   animator;
        bool                       visible = true;

        static std::string getID() { return "Skinned Mesh Renderer"; }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;

            // ── Load primary model (skinned mesh + embedded clips) ──────────────
            std::string modelPath = data.value("model", "");
            if (!modelPath.empty()) {
                auto result = AnimationLoader::load(modelPath);
                if (result.mesh) {
                    skinnedMesh = result.mesh;
                    clips       = std::move(result.clips);
                    animator.init(result.rootNode,
                                  result.globalInverse,
                                  &skinnedMesh->boneInfoMap,
                                  skinnedMesh->boneCounter);
                    printf("[DIAG][SMR] Loaded mesh: %s submeshes=%zu\n", modelPath.c_str(), skinnedMesh->submeshes.size());
                } else {
                    printf("[DIAG][SMR] FAILED to load mesh: %s\n", modelPath.c_str());
                }
            }

            // ── Load additional animation-only files ────────────────────────────
            if (data.contains("animations") && data["animations"].is_array()) {
                for (auto& path : data["animations"]) {
                    if (path.is_string()) {
                        auto extra = AnimationLoader::load(path.get<std::string>());
                        for (auto& c : extra.clips)
                            clips.push_back(std::move(c));
                    }
                }
            }

            animator.loadClips(clips);

            // ── Resolve materials by name from AssetLoader ───────────────────────
            if (data.contains("materials") && data["materials"].is_array()) {
                for (auto& matName : data["materials"]) {
                    if (matName.is_string())
                        materials.push_back(AssetLoader<Material>::get(matName.get<std::string>()));
                }
            } else if (data.contains("material")) {
                // Fallback for single material
                std::string matName = data.value("material", "");
                if (!matName.empty())
                    materials.push_back(AssetLoader<Material>::get(matName));
            }

            // ── Visibility flag (used by first-person camera) ───────────────────
            visible = data.value("visible", visible);

            // ── Start the default clip ──────────────────────────────────────────
            std::string defaultClip = data.value("clip", "");
            if (!defaultClip.empty())
                animator.play(defaultClip);
        }

        ~SkinnedMeshRendererComponent() override {
            delete skinnedMesh;
            skinnedMesh = nullptr;
            // materials are NOT owned by this component; AssetLoader owns them
        }
    };

} // namespace our
