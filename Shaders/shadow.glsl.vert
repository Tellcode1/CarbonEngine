// output: Shaders/shadow.vert.spv stage: vert name: shadow/vert

#version 450 core

layout (location = 0) in vec3 v_vertices;

layout(set = 0, binding = 0) uniform uniform_buffer {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 cameraposition;
    vec3 lightposition;
    vec4 lightcolor;
    vec4 color;
} ub;

void main() {
    vec4 modeled_position = ub.model * vec4(v_vertices, 1.0);
    gl_Position = ub.projection * ub.view * modeled_position;
}