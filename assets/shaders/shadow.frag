#version 330 core

// Shadow pass fragment shader.
// We write no color output — the GPU writes gl_FragDepth automatically.
// This depth texture becomes the shadow map sampled in light.frag.

void main() {}
