#include "animator.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace our {

    void Animator::init(const NodeData& rootNode,
                        const glm::mat4& globalInverse,
                        std::unordered_map<std::string, BoneInfo>* boneMap,
                        int boneCount)
    {
        mRoot          = rootNode;
        mGlobalInverse = globalInverse;
        mBoneMap       = boneMap;

        // Pre-fill with identity matrices so any un-animated bone is a no-op.
        int count = std::min(boneCount, MAX_BONES);
        mFinalMatrices.assign(count, glm::mat4(1.f));
    }

    void Animator::loadClips(const std::vector<AnimationClip>& clips) {
        for (const auto& c : clips)
            mClips[c.name] = c;
    }

    void Animator::play(const std::string& name, bool loop, float speed, float crossFadeOverride) {
        auto it = mClips.find(name);
        if (it == mClips.end()) return;
        
        float duration = (crossFadeOverride < 0.0f) ? mCrossFadeDuration : crossFadeOverride;

        // If switching to a new clip, start a crossfade
        if (mCurrentName != name) {
            mPrevious = mCurrent;
            mPreviousTime = mTime;
            mCrossFadeTime = 0.f;
            mCurrentTransitionDuration = duration;
            mTime = 0.f;
        }

        mCurrent     = &it->second;
        mCurrentName = name;
        mLoop        = loop;
        mPlaybackSpeed = speed;
    }

    void Animator::update(float deltaTime) {
        if (!mCurrent || !mBoneMap) return;

        // Advance current time
        mTime += deltaTime * mCurrent->ticksPerSecond * mPlaybackSpeed;
        if (mLoop) {
            mTime = std::fmod(mTime, mCurrent->duration);
            if (mTime < 0.f) mTime += mCurrent->duration;
        } else {
            mTime = std::min(mTime, mCurrent->duration);
        }

        // Advance previous time (for blending)
        if (mPrevious) {
            // We use the new clip's speed for the blend phase to keep them roughly in sync
            // but we use the previous clip's ticksPerSecond for correct sampling.
            mPreviousTime += deltaTime * mPrevious->ticksPerSecond * mPlaybackSpeed;
            if (mLoop) { 
                mPreviousTime = std::fmod(mPreviousTime, mPrevious->duration);
                if (mPreviousTime < 0.f) mPreviousTime += mPrevious->duration;
            } else {
                mPreviousTime = std::min(mPreviousTime, mPrevious->duration);
            }

            mCrossFadeTime += deltaTime;
            if (mCrossFadeTime >= mCurrentTransitionDuration) {
                mPrevious = nullptr;
            }
        }

        // Walk the node hierarchy to fill mFinalMatrices
        traverse(mRoot, glm::mat4(1.f));
    }

    // -----------------------------------------------------------------------
    // Private: hierarchy traversal
    // -----------------------------------------------------------------------
    void Animator::traverse(const NodeData& node, const glm::mat4& parentTransform) {
        glm::mat4 nodeTransform = node.defaultTransform;

        // If either clip has data for this node, compute the animated TRS.
        if (findChannel(node.name) || (mPrevious && findChannel(node.name, mPrevious))) {
            BonePose poseCur = getPose(mCurrent, mTime, node);
            
            glm::vec3 pos   = poseCur.position;
            glm::quat rot   = poseCur.rotation;
            glm::vec3 scale = poseCur.scale;

            if (mPrevious) {
                BonePose posePre = getPose(mPrevious, mPreviousTime, node);
                
                // Use smoothstep for a more cinematic "ease-in-out" transition
                float t = glm::clamp(mCrossFadeTime / mCurrentTransitionDuration, 0.0f, 1.0f);
                float alpha = t * t * (3.0f - 2.0f * t); // Smoothstep

                pos   = glm::mix(posePre.position, poseCur.position, alpha);
                rot   = glm::slerp(posePre.rotation, poseCur.rotation, alpha);
                scale = glm::mix(posePre.scale, poseCur.scale, alpha);
            }

            // Suppress root motion (vertical translation) if requested
            if (mSuppressRootMotion) {
                std::string lowerName = node.name;
                for(auto &c : lowerName) c = (char)std::tolower(c);
                
                if (node.name == mRoot.name || 
                    lowerName.find("root") != std::string::npos || 
                    lowerName.find("armature") != std::string::npos ||
                    lowerName.find("hips") != std::string::npos ||
                    lowerName.find("pelvis") != std::string::npos) {
                    pos.y = 0.0f; // Lock vertical height
                }
            }

            nodeTransform = glm::translate(glm::mat4(1.f), pos)
                          * glm::mat4_cast(rot)
                          * glm::scale(glm::mat4(1.f), scale);
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        // If this node is a bone, compute its final skinning matrix.
        auto it = mBoneMap->find(node.name);
        if (it != mBoneMap->end()) {
            int idx = it->second.id;
            if (idx >= 0 && idx < (int)mFinalMatrices.size()) {
                mFinalMatrices[idx] =
                    mGlobalInverse * globalTransform * it->second.offsetMatrix;
            }
        }

        for (const auto& child : node.children)
            traverse(child, globalTransform);
    }

    // Returns the animated TRS for a node in a specific clip at a specific time.
    // Correctly falls back to the node's defaultTransform (bind pose) if no channel exists.
    Animator::BonePose Animator::getPose(const AnimationClip* clip, float time, const NodeData& node) const {
        const BoneChannel* ch = findChannel(node.name, clip);
        if (ch) {
            return { interpPosition(*ch, time), interpRotation(*ch, time), interpScale(*ch, time) };
        }
        
        // Fallback: extract TRS from the node's default transformation matrix (the bind pose)
        BonePose pose;
        glm::mat4 m = node.defaultTransform;
        pose.position = glm::vec3(m[3]);
        pose.scale    = glm::vec3(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2])));
        
        // Remove scale from rotation columns to extract pure rotation
        glm::mat3 rotM;
        rotM[0] = glm::vec3(m[0]) / pose.scale.x;
        rotM[1] = glm::vec3(m[1]) / pose.scale.y;
        rotM[2] = glm::vec3(m[2]) / pose.scale.z;
        pose.rotation = glm::quat_cast(rotM);
        
        return pose;
    }

    const BoneChannel* Animator::findChannel(const std::string& name, const AnimationClip* clip) const {
        const AnimationClip* target = clip ? clip : mCurrent;
        if (!target) return nullptr;
        for (const auto& ch : target->channels)
            if (ch.boneName == name) return &ch;
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // Interpolation helpers — all use binary search for O(log n) lookup
    // -----------------------------------------------------------------------
    glm::vec3 Animator::interpPosition(const BoneChannel& ch, float t) const {
        if (ch.positions.size() == 1) return ch.positions[0].value;
        for (size_t i = 0; i + 1 < ch.positions.size(); i++) {
            if (t < ch.positions[i + 1].time) {
                float span = ch.positions[i + 1].time - ch.positions[i].time;
                float f    = (span > 0.f) ? (t - ch.positions[i].time) / span : 0.f;
                return glm::mix(ch.positions[i].value, ch.positions[i + 1].value, f);
            }
        }
        return ch.positions.back().value;
    }

    glm::quat Animator::interpRotation(const BoneChannel& ch, float t) const {
        if (ch.rotations.size() == 1) return ch.rotations[0].value;
        for (size_t i = 0; i + 1 < ch.rotations.size(); i++) {
            if (t < ch.rotations[i + 1].time) {
                float span = ch.rotations[i + 1].time - ch.rotations[i].time;
                float f    = (span > 0.f) ? (t - ch.rotations[i].time) / span : 0.f;
                return glm::normalize(glm::slerp(ch.rotations[i].value,
                                                  ch.rotations[i + 1].value, f));
            }
        }
        return ch.rotations.back().value;
    }

    glm::vec3 Animator::interpScale(const BoneChannel& ch, float t) const {
        if (ch.scales.size() == 1) return ch.scales[0].value;
        for (size_t i = 0; i + 1 < ch.scales.size(); i++) {
            if (t < ch.scales[i + 1].time) {
                float span = ch.scales[i + 1].time - ch.scales[i].time;
                float f    = (span > 0.f) ? (t - ch.scales[i].time) / span : 0.f;
                return glm::mix(ch.scales[i].value, ch.scales[i + 1].value, f);
            }
        }
        return ch.scales.back().value;
    }

} // namespace our
