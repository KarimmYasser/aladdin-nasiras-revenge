#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace our {

    // Maximum number of bones that can influence a single vertex.
    // 4 is the standard GPU limit (fits into an ivec4 / vec4 attribute pair).
    static constexpr int MAX_BONE_INFLUENCE = 4;

    // Extended vertex that carries skinning data on top of the usual
    // position / color / tex_coord / normal attributes.
    //
    // Attribute layout (matches skinned.vert):
    //   location 0  ->  position     (vec3)
    //   location 1  ->  color        (vec4 ubyte)
    //   location 2  ->  tex_coord    (vec2)
    //   location 3  ->  normal       (vec3)
    //   location 4  ->  boneIDs      (ivec4)   -- glVertexAttribIPointer
    //   location 5  ->  boneWeights  (vec4)    -- glVertexAttribPointer
    struct SkinnedVertex {
        glm::vec3 position    = {0, 0, 0};
        glm::vec4 color       = {1, 1, 1, 1};   // rgba float (matches how the shader reads it)
        glm::vec2 tex_coord   = {0, 0};
        glm::vec3 normal      = {0, 1, 0};

        // Indices into the finalBoneMatrices[] uniform array.
        // -1 means "no bone" – the shader skips it.
        int boneIDs[MAX_BONE_INFLUENCE]     = {-1, -1, -1, -1};
        // Corresponding blend weights; must sum to 1.0 across all active influences.
        float boneWeights[MAX_BONE_INFLUENCE] = { 0,  0,  0,  0};

        // Utility: add one bone influence (ignores if all slots are full).
        void addBoneInfluence(int boneID, float weight) {
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                if (boneIDs[i] < 0) {
                    boneIDs[i]     = boneID;
                    boneWeights[i] = weight;
                    return;
                }
            }
            // All slots full – silently drop. Assimp's aiProcess_LimitBoneWeights
            // guarantees at most MAX_BONE_INFLUENCE influences per vertex, so this
            // should never happen when the importer flag is used.
        }
    };

} // namespace our
