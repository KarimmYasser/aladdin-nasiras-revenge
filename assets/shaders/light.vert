#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

// Outputs to the fragment shader
out Varyings {
    vec3 frag_pos;    // World-space fragment position (used to compute light direction & attenuation)
    vec3 normal;      // World-space surface normal   (used for diffuse & specular calculations)
    vec2 tex_coord;
    vec4 color;
    vec4 frag_pos_light_space;  // Fragment position in the primary light's clip space (for shadow map lookup)
} vs_out;

uniform mat4 transform;
uniform mat4 model;
uniform mat3 normal_mat; // transpose for the model
uniform mat4 light_space_matrix; // Primary directional light VP matrix (set by ForwardRenderer each frame)

void main() {
    // Place the vertex in clip space for rasterization
    gl_Position = transform * vec4(position, 1.0);

    // Compute world-space position for the fragment shader
    // (used for direction-to-light and distance-based attenuation)
    vs_out.frag_pos  = vec3(model * vec4(position, 1.0));

    // Transform the normal to world space.
    // We normalize here to remove any scale artifacts; the fragment shader will
    // renormalize after interpolation to handle perspective distortion correctly.
    vs_out.normal    = normalize(normal_mat * normal);

    vs_out.tex_coord = tex_coord;
    vs_out.color     = color;

    // Project the world-space position into the primary light's clip space.
    // The fragment shader will use this to sample the shadow map.
    vs_out.frag_pos_light_space = light_space_matrix * vec4(vs_out.frag_pos, 1.0);
}
