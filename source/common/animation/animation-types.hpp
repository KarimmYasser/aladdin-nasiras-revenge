#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace our {

    // -----------------------------------------------------------------------
    // Individual key-frame types
    // Each stores a value and the time (in animation ticks) at which it
    // should be sampled.  The Animator interpolates between consecutive pairs.
    // -----------------------------------------------------------------------

    struct KeyPosition {
        glm::vec3 value;
        float     time;   // in ticks
    };

    struct KeyRotation {
        glm::quat value;  // quaternion – SLERP is used for interpolation
        float     time;
    };

    struct KeyScale {
        glm::vec3 value;
        float     time;
    };

    // -----------------------------------------------------------------------
    // BoneChannel
    // All key-frames for a single bone in a single animation clip.
    // Matches one aiNodeAnim inside Assimp's aiAnimation.
    // -----------------------------------------------------------------------
    struct BoneChannel {
        std::string boneName;                  // must match the node name in the skeleton hierarchy
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale>    scales;
    };

    // -----------------------------------------------------------------------
    // AnimationClip
    // One complete animation (e.g. "idle", "walk", "attack").
    // Assimp stores each aiAnimation as one clip; we mirror that 1-to-1.
    //
    // duration       – total length in ticks
    // ticksPerSecond – convert to real time: realTime = tick / ticksPerSecond
    // -----------------------------------------------------------------------
    struct AnimationClip {
        std::string              name;
        float                    duration;        // ticks
        float                    ticksPerSecond;  // usually 24 or 30
        std::vector<BoneChannel> channels;        // one entry per animated bone
    };

    // -----------------------------------------------------------------------
    // NodeData
    // A lightweight copy of one aiNode from Assimp's scene graph.
    // The Animator traverses this hierarchy each frame to propagate transforms
    // from parent bones down to children.
    // -----------------------------------------------------------------------
    struct NodeData {
        std::string          name;
        glm::mat4            defaultTransform;  // aiNode::mTransformation converted to glm
        std::vector<NodeData> children;
    };

} // namespace our
