// output: Shaders/vert.text.spv stage: vert name: ctext/vert

#version 450

layout(location=0) in
vec3 v_vertices;

layout(location=1) in
vec2 v_uv;

layout(push_constant) uniform push_constant
{
    mat4 pc_model;
} pc;

layout(location=0) out
vec2 f_uv;

void main() {
    gl_Position = pc.pc_model * vec4(v_vertices, 1.0);
    f_uv = v_uv;
}
