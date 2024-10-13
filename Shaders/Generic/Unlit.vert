// output: Shaders/Generic/Unlit.vert.spv stage: vert name: Unlit/vert

#version 450 core

layout(location=0) in
vec3 v_vertices;

layout(location=1) in
vec2 v_tex_coords;

layout(location=0) out
vec2 f_tex_coords;

layout (push_constant) uniform push_constants {
    mat4 projection_view;
    mat4 model;
} ub;

void main() {
    f_tex_coords = v_tex_coords;
    gl_Position = ub.projection_view * vec4(v_vertices, 1.0);
}