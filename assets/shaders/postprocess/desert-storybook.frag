#version 330

// Custom post-process for Level 1: warm "Agrabah storybook" look with film grain
// and a tinted edge falloff (multiplicative warm sand), not the same math as vignette.frag.

uniform sampler2D tex;

in vec2 tex_coord;
out vec4 frag_color;

float grain_noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec3 col = texture(tex, tex_coord).rgb;

    // Gentle S-curve contrast
    col = col * col * (3.0 - 2.0 * col);

    // Push mids/highlights toward warm gold, keep shadows closer to neutral
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    vec3 warm = col * vec3(1.06, 1.02, 0.90);
    float warm_mix = smoothstep(0.08, 0.65, luma) * 0.5;
    col = mix(col, warm, warm_mix);

    // Edge tint: multiply by warm sand color toward screen border (distinct from vignette's divide)
    vec2 ndc = tex_coord * 2.0 - 1.0;
    float edge = smoothstep(0.25, 1.0, length(ndc));
    vec3 sand = vec3(1.0, 0.88, 0.72);
    col = mix(col, col * sand, edge * 0.45);

    // High-frequency film grain
    float g = (grain_noise(tex_coord * vec2(1920.0, 1080.0)) - 0.5) * 0.035;
    col += g;

    frag_color = vec4(clamp(col, 0.0, 1.0), 1.0);
}
