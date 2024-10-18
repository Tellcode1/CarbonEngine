// output: Shaders/Generic/Unlit.frag.spv stage: frag name: Unlit/frag

#version 450 core

layout(location = 0) out vec4 o_color;

layout (location = 0) in
vec2 f_tex_coords;

layout(set = 0, binding = 1) uniform sampler2D f_texture;

void main() {
    o_color = texture(f_texture, f_tex_coords);
}