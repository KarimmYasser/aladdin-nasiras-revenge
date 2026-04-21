#version 330 core

// Shadow pass vertex shader.
// Renders the scene from the light's point of view to produce a depth map.
// Only vertex position matters — no normals, tex-coords, or color needed.

layout(location = 0) in vec3 position;

uniform mat4 light_space_matrix; // light's ViewProjection (orthographic for directional)
uniform mat4 model;              // object-to-world

void main() {
    gl_Position = light_space_matrix * model * vec4(position, 1.0);
}
