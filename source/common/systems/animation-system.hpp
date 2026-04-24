#pragma once

#include "../ecs/world.hpp"
#include "../components/animator-component.hpp"
#include "../components/skinned-mesh-renderer.hpp"

namespace our {

    // AnimationSystem
    // ---------------
    // Advances every animated component in the world by deltaTime each frame.
    // Must be called BEFORE ForwardRenderer::render() so the finalBoneMatrices[]
    // array is up to date when the GPU draw calls happen.
    class AnimationSystem {
    public:
        void update(World* world, float deltaTime) {
            for (auto entity : world->getEntities()) {
                // New merged component path
                if (auto* smr = entity->getComponent<SkinnedMeshRendererComponent>())
                    smr->animator.update(deltaTime);
                // Legacy AnimatorComponent path (kept for backward compat)
                if (auto* anim = entity->getComponent<AnimatorComponent>())
                    anim->animator.update(deltaTime);
            }
        }
    };

} // namespace our
