#version 330 core

out vec4 frag_color;

// In this shader, we want to draw a checkboard where the size of each tile is (size x size).
// The color of the top-left most tile should be "colors[0]" and the 2 tiles adjacent to it
// should have the color "colors[1]".

//TODO: (Req 1) Finish this shader.

uniform int size = 32;
uniform vec3 colors[2];

void main(){
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int tileX = pixel.x / size;
    int tileY = pixel.y / size;
    int checker = (tileX + tileY) % 2;
    frag_color = vec4(colors[checker], 1.0);
}