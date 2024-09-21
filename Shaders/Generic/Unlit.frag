// output: Shaders/Generic/Unlit.frag.spv stage: frag name: Unlit/frag

#version 450 core

layout(location = 0) out vec4 o_color;

layout (location = 0) in
vec2 f_tex_coords;

void main() {
    o_color = vec4(1.0, 1.0, 1.0, 1.0);
}