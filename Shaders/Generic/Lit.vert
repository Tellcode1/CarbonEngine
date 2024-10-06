// output: Shaders/vert.spv stage: vert name: test/vert

#version 450 core

// < vertices, normals, texcoords >

layout(location=0) in
vec3 v_vertices;

layout(location=1) in
vec3 v_normal;

layout(location=2) in
vec3 v_tangent;

layout(location=3) in
vec3 v_bitangent;

layout(location=4) in
vec2 v_tex_coords;

layout (location = 0) out
vec2 f_tex_coords;

layout (location = 1) out
vec3 f_normal;

layout (location = 2) out
vec3 TangentFragPos;

layout (location = 3) out
vec3 TangentLightPos;

layout (location = 4) out
vec3 TangentViewPos;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

layout(set = 0, binding = 0, std140) uniform uniform_buffer {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 cameraposition;
    vec3 lightposition;
    vec3 lightcolor;
    vec3 color;
    Material mat;
} ub;

layout (push_constant) uniform push_constants {
    vec3 lightposition;
};

layout (set = 0, binding = 1) uniform sampler2D image; // descriptor image
layout (set = 0, binding = 2) uniform sampler2D normalmap;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(ub.model)));
    vec3 N = normalize(normalMatrix * v_normal);
    vec3 T = normalize(normalMatrix * v_tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    vec4 modeled_position = ub.model * vec4(v_vertices, 1.0);

    f_normal = v_normal;
    f_tex_coords = v_tex_coords;

    TangentFragPos  = TBN * modeled_position.xyz;
    TangentLightPos = TBN * ub.lightposition.xyz;
    TangentViewPos  = TBN * ub.cameraposition.xyz;

    gl_Position = ub.projection * ub.view * modeled_position;
}