#pragma once

#include <glm/glm.hpp>

namespace our {

    // Stores everything the Animator needs to know about one bone.
    //
    // - id          : index of this bone inside the finalBoneMatrices[] array that
    //                 is uploaded to the shader each frame.
    // - offsetMatrix: the "inverse bind-pose" matrix exported by the DCC tool
    //                 (Blender, Maya, etc.).  It transforms a vertex from mesh
    //                 local-space into the bone's local-space when the skeleton
    //                 is in its rest pose.
    //
    // Final per-vertex transform =
    //     globalInverseTransform * nodeGlobalTransform * offsetMatrix
    struct BoneInfo {
        int       id;             // slot in finalBoneMatrices[]
        glm::mat4 offsetMatrix;   // inverse bind-pose (from Assimp aiBone::mOffsetMatrix)
    };

} // namespace our
