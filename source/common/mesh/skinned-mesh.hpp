#pragma once

#include <glad/gl.h>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <string>
#include "skinned-vertex.hpp"
#include "bone-info.hpp"

namespace our {

    // Attribute locations – the first 4 match the regular Mesh so the same
    // material/shader infrastructure can be reused for lighting uniforms.
    // Locations 4 & 5 are the extra skinning inputs consumed only by skinned.vert.
    #define ATTRIB_LOC_POSITION     0
    #define ATTRIB_LOC_COLOR        1
    #define ATTRIB_LOC_TEXCOORD     2
    #define ATTRIB_LOC_NORMAL       3
    #define ATTRIB_LOC_BONE_IDS     4   // ivec4  – glVertexAttribIPointer
    #define ATTRIB_LOC_BONE_WEIGHTS 5   // vec4   – glVertexAttribPointer

    // Submesh definition for multi-material models
    struct Submesh {
        int   elementCount;
        void* elementOffset;
    };

    // SkinnedMesh
    // -----------
    // Identical to Mesh but stores SkinnedVertex data (6 attributes instead of 4).
    // The bone matrices are NOT stored here; they live in the Animator and are
    // uploaded to the shader as a uniform array each frame.
    //
    // boneInfoMap and boneCounter are filled by AnimationLoader::loadSkinnedMesh()
    // and then read by the Animator to know which matrix slot each bone occupies.
    class SkinnedMesh {
        GLuint  VBO = 0, EBO = 0, VAO = 0;
        GLsizei elementCount = 0;

    public:
        // Filled by the loader: bone name → { id, offsetMatrix }
        std::unordered_map<std::string, BoneInfo> boneInfoMap;
        // Next free slot in finalBoneMatrices[]; incremented by the loader.
        int boneCounter = 0;

        // Support for multiple materials: the mesh can be drawn in multiple parts
        std::vector<Submesh> submeshes;

        SkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                    const std::vector<unsigned int>&   elements,
                    const std::vector<Submesh>&       submeshes = {});

        // Draw the whole mesh or a specific submesh
        void draw();
        void drawSubmesh(int index);

        ~SkinnedMesh();

        // Non-copyable – GPU resources are owned exclusively by this object.
        SkinnedMesh(const SkinnedMesh&)            = delete;
        SkinnedMesh& operator=(const SkinnedMesh&) = delete;
    };

} // namespace our
