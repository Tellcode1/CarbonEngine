// output: Shaders/Generic/Unlit.vert.spv stage: vert name: Unlit/vert

#version 450 core

layout(location=0) in
vec3 v_vertices;

layout(location=1) in
vec2 v_tex_coords;

layout(location=0) out
vec2 f_tex_coords;

layout(set = 0, binding = 0, std140) uniform uniform_buffer {
    mat4 view;
    mat4 projection;
} ub;

layout (push_constant) uniform push_constants {
    mat4 model;
    vec4 color;
} pc;

void main() {
    f_tex_coords = v_tex_coords;
    gl_Position = ub.projection * ub.view * pc.model * vec4(v_vertices, 1.0);
}