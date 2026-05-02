#version 330

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D tex;

void main() {
    // FFmpeg rows are top-down; GL sample space expects bottom-up for our fullscreen.vert
    vec2 uv = vec2(tex_coord.x, 1.0 - tex_coord.y);
    frag_color = texture(tex, uv);
}
