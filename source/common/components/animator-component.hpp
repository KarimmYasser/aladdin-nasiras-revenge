#pragma once

#include "../ecs/component.hpp"
#include "../animation/animation-loader.hpp"
#include "../animation/animator.hpp"
#include "../mesh/skinned-mesh.hpp"

namespace our {

    // AnimatorComponent
    // -----------------
    // Attach to any entity that should display a skinned, animated mesh.
    // The component owns the SkinnedMesh (loaded from a DAE/FBX file) and
    // drives the Animator each frame via AnimationSystem::update().
    //
    // JSON example (inside an entity's "components" array):
    // {
    //     "type": "Animator",
    //     "model": "assets/models/Aladdin/aladdin_costume_basic.dae",
    //     "clip":  "idle"
    // }
    class AnimatorComponent : public Component {
    public:
        SkinnedMesh*             skinnedMesh = nullptr;  // owned; deleted in destructor
        std::vector<AnimationClip> clips;
        Animator                 animator;

        static std::string getID() { return "Animator"; }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;

            std::string modelPath = data.value("model", "");
            if (modelPath.empty()) return;

            // Load the model (blocks on first load — consider caching in AssetLoader later)
            auto result = AnimationLoader::load(modelPath);
            if (!result.mesh) return;

            // Transfer ownership
            skinnedMesh = result.mesh;
            clips       = std::move(result.clips);

            // Load additional clips if specified
            if (data.contains("animations") && data["animations"].is_array()) {
                for (auto& path : data["animations"]) {
                    if (path.is_string()) {
                        auto extra = AnimationLoader::load(path.get<std::string>());
                        // Append found clips to our list
                        for (auto& c : extra.clips) {
                            clips.push_back(std::move(c));
                        }
                    }
                }
            }

            // Wire the Animator
            animator.init(result.rootNode,
                          result.globalInverse,
                          &skinnedMesh->boneInfoMap,
                          skinnedMesh->boneCounter);
            animator.loadClips(clips);

            // Start the default clip if specified
            std::string defaultClip = data.value("clip", "");
            if (!defaultClip.empty())
                animator.play(defaultClip);
        }

        ~AnimatorComponent() override {
            delete skinnedMesh;
            skinnedMesh = nullptr;
        }
    };

} // namespace our
