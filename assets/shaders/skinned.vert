#version 330 core

// ── Vertex attributes ──────────────────────────────────────────────────────
layout(location = 0) in vec3  position;
layout(location = 1) in vec4  color;
layout(location = 2) in vec2  tex_coord;
layout(location = 3) in vec3  normal;
layout(location = 4) in ivec4 boneIDs;      // indices into finalBoneMatrices[]
layout(location = 5) in vec4  boneWeights;  // blend weights (sum == 1.0)

// ── Skinning uniforms ───────────────────────────────────────────────────────
const int MAX_BONES          = 64;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBoneMatrices[MAX_BONES];  // set by ForwardRenderer each frame

// ── Standard transform uniforms (same names as light.vert) ─────────────────
uniform mat4 transform;          // VP * model  (clip-space)
uniform mat4 model;              // world-space model matrix
uniform mat3 normal_mat;         // transpose(inverse(model)) for normal transform
uniform vec2 uv_multiplier = vec2(1.0, 1.0);
uniform mat4 light_space_matrix; // primary directional light VP (for shadow map)

// ── Outputs to light.frag (identical interface to light.vert) ───────────────
out Varyings {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coord;
    vec4 color;
    vec4 frag_pos_light_space;
} vs_out;

void main() {
    // ── Accumulate skinned position and normal ──────────────────────────────
    vec4 skinnedPos    = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int boneIdx = boneIDs[i];
        if (boneIdx < 0) continue;          // -1 == no bone influence
        if (boneIdx >= MAX_BONES) continue; // safety guard

        mat4 bm = finalBoneMatrices[boneIdx];
        skinnedPos    += boneWeights[i] * (bm * vec4(position, 1.0));
        skinnedNormal += boneWeights[i] * (mat3(bm) * normal);
    }

    // Guard: if all weights were zero (static mesh), fall back to original position
    if (dot(skinnedPos, skinnedPos) < 0.0001) {
        skinnedPos = vec4(position, 1.0);
        skinnedNormal = normal;
    }

    // Final texture path: use white tint to see raw albedo
    vs_out.color      = vec4(1.0);
    vs_out.tex_coord  = vec2(tex_coord.x, 1.0 - tex_coord.y) * uv_multiplier;
    vs_out.normal     = normalize(normal_mat * skinnedNormal);
    vs_out.frag_pos   = vec3(model * skinnedPos);
    vs_out.frag_pos_light_space = light_space_matrix * vec4(vs_out.frag_pos, 1.0);

    gl_Position = transform * skinnedPos;
}
