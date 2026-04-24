#pragma once

#include "../mesh/skinned-mesh.hpp"
#include "animation-types.hpp"
#include <string>
#include <vector>

// Forward declare so we don't pull all of Assimp into every translation unit
struct aiNode;

namespace our::AnimationLoader {

    // Bundles everything the Animator needs, returned by load().
    struct LoadResult {
        SkinnedMesh*           mesh = nullptr;   // heap-allocated, caller owns it
        NodeData               rootNode;          // full scene-node hierarchy copy
        glm::mat4              globalInverse{1.f};// inverse of the root transform
        std::vector<AnimationClip> clips;         // every aiAnimation in the file
    };

    // Load a skinned mesh from a DAE / FBX file using Assimp.
    // Returns a filled LoadResult; mesh is nullptr on failure.
    LoadResult load(const std::string& path);

} // namespace our::AnimationLoader
