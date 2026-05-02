#include "animation-loader.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <cctype>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

namespace {

    // Clean up Assimp bone names (remove namespace prefixes like "Armature_")
    std::string cleanName(const std::string& name) {
        // For Mixamo/common rigs, just return the name as-is
        // If there are prefixes (e.g., "Armature_Hips"), extract the suffix
        size_t pos = name.find_last_of('_');
        if (pos != std::string::npos && pos + 1 < name.length()) {
            // Check if what comes before the last '_' looks like a namespace
            std::string prefix = name.substr(0, pos);
            if (prefix.find_last_of('_') == std::string::npos) {
                // Only one underscore, likely a namespace prefix
                return name.substr(pos + 1);
            }
        }
        return name;
    }

    // Assimp uses row-major, GLM uses column-major — transpose on conversion.
    glm::mat4 toGlm(const aiMatrix4x4& m) {
        return glm::transpose(glm::make_mat4(&m.a1));
    }

    // Recursively copy the Assimp node tree into our lightweight NodeData.
    our::NodeData buildHierarchy(const aiNode* src) {
        our::NodeData node;
        node.name             = src->mName.C_Str();
        node.defaultTransform = toGlm(src->mTransformation);
        node.children.reserve(src->mNumChildren);
        for (unsigned int i = 0; i < src->mNumChildren; i++)
            node.children.push_back(buildHierarchy(src->mChildren[i]));
        return node;
    }

} // anonymous namespace

namespace our::AnimationLoader {

    LoadResult load(const std::string& path) {
        LoadResult result;

        Assimp::Importer importer;
        // Fix for "flat" character: Mixamo FBX files are often in cm, Assimp
        // defaults to scaling them by 0.01 to meters. We force 1.0.
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0f);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

        // Debug: Check if file exists and can be opened
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            std::cerr << "[AnimationLoader] FATAL: Cannot open file for reading: " << path << "\n";
            return result;
        }
        f.close();

        // Stage 1: Standard load with full processing
        unsigned int ppFlags = aiProcess_Triangulate |
                               aiProcess_FlipUVs |
                               aiProcess_CalcTangentSpace |
                               aiProcess_GenSmoothNormals |
                               aiProcess_JoinIdenticalVertices |
                               aiProcess_LimitBoneWeights |
                               aiProcess_GlobalScale |
                               aiProcess_PopulateArmatureData;
        const aiScene* scene = importer.ReadFile(path, ppFlags);

        // Stage 2: Minimal fallback for animation-only files
        if (!scene || !scene->mRootNode) {
            scene = importer.ReadFile(path, aiProcess_GlobalScale | aiProcess_PopulateArmatureData);
        }

        // Stage 3: Absolute raw fallback (Last Resort)
        if (!scene || !scene->mRootNode) {
            scene = importer.ReadFile(path, aiProcess_GlobalScale);
        }

        if (!scene || !scene->mRootNode) {
            const char* err = importer.GetErrorString();
            std::cerr << "[AnimationLoader] Failed to load: " << path
                      << "\n  Reason: " << (err && *err ? err : "Unknown (possibly no root node or corrupted file)") << "\n";
            return result;
        }

        // ----------------------------------------------------------------
        // 1. Build the bone map from all meshes (if any) or animations
        // ----------------------------------------------------------------
        std::unordered_map<std::string, BoneInfo> boneInfoMap;
        int boneCounter = 0;
        
        // From meshes
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];
            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* bone = mesh->mBones[b];
                std::string name = cleanName(bone->mName.C_Str());
                if (boneInfoMap.find(name) == boneInfoMap.end()) {
                    boneInfoMap[name] = { boneCounter++, toGlm(bone->mOffsetMatrix) };
                }
            }
        }
        
        // If no meshes, try to find bones from animations
        if (boneInfoMap.empty()) {
             for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
                aiAnimation* ai = scene->mAnimations[a];
                for (unsigned int c = 0; c < ai->mNumChannels; c++) {
                    std::string name = cleanName(ai->mChannels[c]->mNodeName.C_Str());
                    if (boneInfoMap.find(name) == boneInfoMap.end()) {
                        boneInfoMap[name] = { boneCounter++, glm::mat4(1.0f) };
                    }
                }
            }
        }

        // ----------------------------------------------------------------
        // 2. Vertex + index data
        // ----------------------------------------------------------------
        std::vector<SkinnedVertex> vertices;
        std::vector<unsigned int>  indices;
        std::vector<Submesh>       submeshes;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];
            unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());
            unsigned int indexOffset  = static_cast<unsigned int>(indices.size());

            // Base vertex attributes
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                SkinnedVertex v;
                v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                v.normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.f, 1.f, 0.f);
                v.tex_coord = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.f);
                v.color = { 1.f, 1.f, 1.f, 1.f };
                vertices.push_back(v);
            }

            // Indices
            unsigned int meshIndexCount = 0;
            for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
                for (unsigned int j = 0; j < mesh->mFaces[f].mNumIndices; j++) {
                    indices.push_back(vertexOffset + mesh->mFaces[f].mIndices[j]);
                    meshIndexCount++;
                }
            }

            Submesh sub;
            sub.elementCount = static_cast<GLsizei>(meshIndexCount);
            sub.elementOffset = (void*)(uintptr_t)(indexOffset * sizeof(unsigned int));
            submeshes.push_back(sub);

            // Bone weights
            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* bone = mesh->mBones[b];
                std::string boneName = cleanName(bone->mName.C_Str());
                if (boneInfoMap.count(boneName)) {
                    int boneID = boneInfoMap.at(boneName).id;
                    for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                        unsigned int vid = vertexOffset + bone->mWeights[w].mVertexId;
                        float wt = bone->mWeights[w].mWeight;
                        if (vid < vertices.size()) vertices[vid].addBoneInfluence(boneID, wt);
                    }
                }
            }
        }

        // Only create mesh if we have vertices (otherwise it's an animation-only file)
        if (!vertices.empty()) {
            result.mesh = new SkinnedMesh(vertices, indices, submeshes);
            result.mesh->boneInfoMap = boneInfoMap;
            result.mesh->boneCounter = boneCounter;
        }

        // ----------------------------------------------------------------
        // 3. Node hierarchy + global inverse
        // ----------------------------------------------------------------
        result.rootNode      = buildHierarchy(scene->mRootNode);
        result.globalInverse = glm::inverse(toGlm(scene->mRootNode->mTransformation));

        // ----------------------------------------------------------------
        // 4. Animation clips
        // ----------------------------------------------------------------
        result.clips.reserve(scene->mNumAnimations);
        for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
            aiAnimation* ai = scene->mAnimations[a];
            AnimationClip clip;
            
            // Extract clip name
            std::string internalName = ai->mName.C_Str();
            if (internalName == "mixamo.com" || internalName.empty()) {
                std::string filename = path;
                size_t lastSlash = filename.find_last_of("\\/");
                if (lastSlash != std::string::npos) filename = filename.substr(lastSlash + 1);
                size_t lastDot = filename.find_last_of(".");
                if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
                clip.name = filename;
            } else {
                clip.name = internalName;
            }
            
            // Lowercase normalization
            for (auto& c : clip.name) c = (char)std::tolower(c);
            
            clip.duration       = static_cast<float>(ai->mDuration);
            clip.ticksPerSecond = static_cast<float>(ai->mTicksPerSecond != 0.0 ? ai->mTicksPerSecond : 24.0);

            clip.channels.reserve(ai->mNumChannels);
            for (unsigned int c = 0; c < ai->mNumChannels; c++) {
                aiNodeAnim* ch = ai->mChannels[c];
                BoneChannel channel;
                channel.boneName = cleanName(ch->mNodeName.C_Str());

                channel.positions.reserve(ch->mNumPositionKeys);
                for (unsigned int k = 0; k < ch->mNumPositionKeys; k++)
                    channel.positions.push_back({ { ch->mPositionKeys[k].mValue.x, ch->mPositionKeys[k].mValue.y, ch->mPositionKeys[k].mValue.z }, static_cast<float>(ch->mPositionKeys[k].mTime) });

                channel.rotations.reserve(ch->mNumRotationKeys);
                for (unsigned int k = 0; k < ch->mNumRotationKeys; k++)
                    channel.rotations.push_back({ glm::quat(ch->mRotationKeys[k].mValue.w, ch->mRotationKeys[k].mValue.x, ch->mRotationKeys[k].mValue.y, ch->mRotationKeys[k].mValue.z), static_cast<float>(ch->mRotationKeys[k].mTime) });

                channel.scales.reserve(ch->mNumScalingKeys);
                for (unsigned int k = 0; k < ch->mNumScalingKeys; k++)
                    channel.scales.push_back({ { ch->mScalingKeys[k].mValue.x, ch->mScalingKeys[k].mValue.y, ch->mScalingKeys[k].mValue.z }, static_cast<float>(ch->mScalingKeys[k].mTime) });

                clip.channels.push_back(std::move(channel));
            }
            result.clips.push_back(std::move(clip));
        }

        return result;
    }

} // namespace our::AnimationLoader
