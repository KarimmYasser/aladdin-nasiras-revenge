#version 330 core

// Skinned shadow pass vertex shader.
// Applies bone transformations to the vertex position before light-space transform.

layout(location = 0) in vec3 position;
layout(location = 4) in ivec4 boneIDs;
layout(location = 5) in vec4 boneWeights;

const int MAX_BONES = 64;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBoneMatrices[MAX_BONES];
uniform mat4 light_space_matrix;
uniform mat4 model;

void main() {
    vec4 skinnedPos = vec4(0.0);

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int boneIdx = boneIDs[i];
        if (boneIdx < 0) continue;
        if (boneIdx >= MAX_BONES) continue;

        skinnedPos += boneWeights[i] * (finalBoneMatrices[boneIdx] * vec4(position, 1.0));
    }

    // Guard: if all weights were zero, fall back to original position
    if (dot(skinnedPos, skinnedPos) < 0.0001) {
        skinnedPos = vec4(position, 1.0);
    }

    gl_Position = light_space_matrix * model * skinnedPos;
}
