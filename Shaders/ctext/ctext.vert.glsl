// output: Shaders/vert.text.spv stage: vert name: ctext/vert

#version 450

layout(location=0) in
vec3 v_vertices;

layout(location=1) in
vec2 v_uv;

layout(set = 0, binding = 0, std140) uniform camera_buffer {
    mat4 perspective;
    mat4 ortho;
    mat4 view;
} cam_ub;

layout (push_constant) uniform push_constants {
    mat4 model;
    vec4 color;
    vec4 outline_color;
} pc;

layout(location=0) out
vec2 f_uv;

layout(location=1) out
vec4 f_col;

void main() {
    gl_Position = cam_ub.ortho * cam_ub.view * pc.model * vec4(v_vertices, 1.0);
    f_uv = v_uv;
    f_col = pc.color;
}
