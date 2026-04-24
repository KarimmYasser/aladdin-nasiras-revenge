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

        // ── DIAGNOSTIC ─────────────────────────────────────────────
        std::cout << "[DIAG][Animator::init]\n";
        std::cout << "  boneCount=" << boneCount << "  allocated=" << count << "\n";
        std::cout << "  boneMap ptr=" << (void*)boneMap
                  << "  boneMap size=" << (boneMap ? boneMap->size() : 0) << "\n";
        std::cout << "  rootNode name=\"" << mRoot.name << "\"  children=" << mRoot.children.size() << "\n";
        std::cout << "  globalInverse[0][0]=" << mGlobalInverse[0][0]
                  << "  [1][1]=" << mGlobalInverse[1][1]
                  << "  [2][2]=" << mGlobalInverse[2][2]
                  << "  [3][3]=" << mGlobalInverse[3][3] << "\n";
        // ──────────────────────────────────────────────────
    }

    void Animator::loadClips(const std::vector<AnimationClip>& clips) {
        for (const auto& c : clips)
            mClips[c.name] = c;
    }

    void Animator::play(const std::string& name, bool loop) {
        if (mCurrentName == name && mLoop == loop) return; // already playing

        auto it = mClips.find(name);
        if (it == mClips.end()) {
            std::cerr << "[DIAG][Animator::play] Clip NOT FOUND: \"" << name << "\"  totalClips=" << mClips.size() << "\n";
            for (const auto& pair : mClips)
                std::cerr << "  available: \"" << pair.first << "\"\n";
            return;
        }

        std::cout << "[DIAG][Animator::play] Switching to \"" << name << "\"  dur=" << it->second.duration
                  << "  tps=" << it->second.ticksPerSecond
                  << "  channels=" << it->second.channels.size() << "\n";
        mCurrent     = &it->second;
        mCurrentName = name;
        mLoop        = loop;
        // Only reset time when switching to a different clip
        if (mCurrentName != name) mTime = 0.f;
    }

    void Animator::update(float deltaTime) {
        // ── DIAGNOSTIC: one-shot first-frame report ──────────────────────────────
        static int updateCallCount = 0;
        if (updateCallCount < 3) {
            ++updateCallCount;
            std::cout << "[DIAG][Animator::update] call#" << updateCallCount
                      << "  mCurrent=" << (mCurrent ? "\"" + mCurrentName + "\"" : "null")
                      << "  mBoneMap=" << (void*)mBoneMap
                      << "  mBoneMap.size=" << (mBoneMap ? mBoneMap->size() : 0)
                      << "  mFinalMatrices.size=" << mFinalMatrices.size()
                      << "  mTime=" << mTime << "  dt=" << deltaTime << "\n";
        }
        // ────────────────────────────────────────────────────────────────────
        if (!mCurrent || !mBoneMap) return;

        // Advance time in ticks
        mTime += deltaTime * mCurrent->ticksPerSecond;

        if (mLoop) {
            mTime = std::fmod(mTime, mCurrent->duration);
            if (mTime < 0.f) mTime += mCurrent->duration;
        } else {
            mTime = std::min(mTime, mCurrent->duration);
        }

        // Walk the node hierarchy to fill mFinalMatrices
        traverse(mRoot, glm::mat4(1.f));

        // ── DIAGNOSTIC: sample first bone matrix after traverse ───────────────────
        static int postTraverseCount = 0;
        if (postTraverseCount < 2 && !mFinalMatrices.empty()) {
            ++postTraverseCount;
            const glm::mat4& m0 = mFinalMatrices[0];
            std::cout << "[DIAG][Animator::update] bone[0] matrix diagonal="
                      << m0[0][0] <<","<< m0[1][1] <<","<< m0[2][2] <<","<< m0[3][3] << "\n";
            std::cout << "  bone[0] col3(translation)="
                      << m0[3][0] <<","<< m0[3][1] <<","<< m0[3][2] << "\n";
        }
        // ────────────────────────────────────────────────────────────────────
    }

    // -----------------------------------------------------------------------
    // Private: hierarchy traversal
    // -----------------------------------------------------------------------
    void Animator::traverse(const NodeData& node, const glm::mat4& parentTransform) {
        glm::mat4 nodeTransform = node.defaultTransform;

        // If the current clip has a channel for this node, use interpolated TRS.
        const BoneChannel* ch = findChannel(node.name);
        if (ch) {
            glm::vec3 pos   = interpPosition(*ch, mTime);
            glm::quat rot   = interpRotation(*ch, mTime);
            glm::vec3 scale = interpScale   (*ch, mTime);

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

    const BoneChannel* Animator::findChannel(const std::string& name) const {
        if (!mCurrent) return nullptr;
        for (const auto& ch : mCurrent->channels)
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
