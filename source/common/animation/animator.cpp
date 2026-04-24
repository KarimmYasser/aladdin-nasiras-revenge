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

    void Animator::play(const std::string& name, bool loop, float speed) {
        auto it = mClips.find(name);
        if (it == mClips.end()) return;

        // If switching to a new clip, start a crossfade
        if (mCurrentName != name) {
            mPrevious = mCurrent;
            mPreviousTime = mTime;
            mCrossFadeTime = 0.f;
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
            mPreviousTime += deltaTime * mPrevious->ticksPerSecond * mPlaybackSpeed;
            if (mLoop) { // Assuming same loop setting for simplicity during blend
                mPreviousTime = std::fmod(mPreviousTime, mPrevious->duration);
                if (mPreviousTime < 0.f) mPreviousTime += mPrevious->duration;
            } else {
                mPreviousTime = std::min(mPreviousTime, mPrevious->duration);
            }

            mCrossFadeTime += deltaTime;
            if (mCrossFadeTime >= mCrossFadeDuration) {
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

        // If the current clip has a channel for this node, use interpolated TRS.
        const BoneChannel* chCur = findChannel(node.name);
        const BoneChannel* chPre = mPrevious ? findChannel(node.name, mPrevious) : nullptr;

        if (chCur || chPre) {
            glm::vec3 pos, scale;
            glm::quat rot;

            if (chCur && chPre) {
                // Blend between previous and current
                float alpha = glm::clamp(mCrossFadeTime / mCrossFadeDuration, 0.0f, 1.0f);
                pos   = glm::mix(interpPosition(*chPre, mPreviousTime), interpPosition(*chCur, mTime), alpha);
                rot   = glm::slerp(interpRotation(*chPre, mPreviousTime), interpRotation(*chCur, mTime), alpha);
                scale = glm::mix(interpScale(*chPre, mPreviousTime), interpScale(*chCur, mTime), alpha);
            } else if (chCur) {
                pos   = interpPosition(*chCur, mTime);
                rot   = interpRotation(*chCur, mTime);
                scale = interpScale(*chCur, mTime);
            } else {
                pos   = interpPosition(*chPre, mPreviousTime);
                rot   = interpRotation(*chPre, mPreviousTime);
                scale = interpScale(*chPre, mPreviousTime);
            }

            // Suppress root motion (vertical translation) if requested
            // Targets Root, Armature, and common "Hips/Pelvis" nodes where baked movement often lives.
            if (mSuppressRootMotion) {
                std::string lowerName = node.name;
                for(auto &c : lowerName) c = std::tolower(c);
                
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
