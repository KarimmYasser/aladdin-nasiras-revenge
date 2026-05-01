#pragma once

#include "animation-types.hpp"
#include "../mesh/bone-info.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace our {

    class Animator {
    public:
        static constexpr int MAX_BONES = 64;

        // Call once after loading a model.
        // boneMap must outlive this Animator (owned by SkinnedMesh).
        void init(const NodeData& rootNode,
                  const glm::mat4& globalInverse,
                  std::unordered_map<std::string, BoneInfo>* boneMap,
                  int boneCount);

        // Register the animation clips extracted by AnimationLoader.
        void loadClips(const std::vector<AnimationClip>& clips);

        // Start playing a clip by name.
        // loop=true              → clip restarts when it ends
        // loop=false             → clip freezes on the last frame
        // speed                  → playback speed multiplier
        // crossFadeOverride < 0  → use default crossFadeDuration
        void play(const std::string& name, bool loop = true, float speed = 1.0f, float crossFadeOverride = -1.0f);

        // Advance time and recompute finalBoneMatrices. Call every frame.
        void update(float deltaTime);

        // The array uploaded to the GPU each frame as finalBoneMatrices[].
        const std::vector<glm::mat4>& getFinalBoneMatrices() const { return mFinalMatrices; }

        // Name of the currently playing clip ("" if none).
        const std::string& currentClipName() const { return mCurrentName; }

        // Returns true if a clip with the given name exists.
        bool hasClip(const std::string& name) const { return mClips.find(name) != mClips.end(); }

        // Returns the list of all available clips.
        const std::unordered_map<std::string, AnimationClip>& getClips() const { return mClips; }

        // Set/Get the playback speed multiplier
        void setPlaybackSpeed(float speed) { mPlaybackSpeed = speed; }
        float getPlaybackSpeed() const { return mPlaybackSpeed; }

        // Set the crossfade duration in seconds
        void setCrossFadeDuration(float duration) { mCrossFadeDuration = duration; }

        // Root motion suppression (prevents double-jumping if animation has baked movement)
        void setSuppressRootMotion(bool suppress) { mSuppressRootMotion = suppress; }

    private:
        bool mSuppressRootMotion = false;
        // Scene hierarchy
        NodeData  mRoot;
        glm::mat4 mGlobalInverse{1.f};

        // Bone data owned by the SkinnedMesh
        std::unordered_map<std::string, BoneInfo>* mBoneMap = nullptr;

        // Clips indexed by name
        std::unordered_map<std::string, AnimationClip> mClips;

        // Playback state
        const AnimationClip* mCurrent     = nullptr;
        std::string          mCurrentName;
        float                mTime          = 0.f;
        bool                 mLoop          = true;
        float                mPlaybackSpeed = 1.0f;

        // Crossfade state
        const AnimationClip* mPrevious = nullptr;
        float                mPreviousTime = 0.f;
        float                mCrossFadeTime = 0.f;
        float                mCurrentTransitionDuration = 0.2f; // Actual duration for active transition
        float                mCrossFadeDuration = 0.2f;         // Default duration for new transitions

        // Output – MAX_BONES identity matrices initially
        std::vector<glm::mat4> mFinalMatrices;

        // Hierarchy traversal
        void traverse(const NodeData& node, const glm::mat4& parentTransform);

        // Returns the BoneChannel for a given node name in the current clip, or nullptr.
        const BoneChannel* findChannel(const std::string& name, const AnimationClip* clip = nullptr) const;

        // Key-frame interpolation helpers
        struct BonePose {
            glm::vec3 position;
            glm::quat rotation;
            glm::vec3 scale;
        };

        BonePose getPose(const AnimationClip* clip, float time, const NodeData& node) const;
        glm::vec3 interpPosition(const BoneChannel& ch, float t) const;
        glm::quat interpRotation(const BoneChannel& ch, float t) const;
        glm::vec3 interpScale   (const BoneChannel& ch, float t) const;
    };

} // namespace our
