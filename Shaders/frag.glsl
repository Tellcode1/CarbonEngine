// output: Shaders/frag.spv stage: frag name: test/frag

#version 450 core

layout(location = 0) out vec4 o_color;

layout (location = 0) in
vec2 f_tex_coords;

layout (location = 1) in
vec3 f_normal;

layout (location = 2) in
vec3 TangentFragPos;

layout (location = 3) in
vec3 TangentLightPos;

layout (location = 4) in
vec3 TangentViewPos;

#define M_PI 3.1415926535897932384626433832795

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

const float ambientStrength = 0.1;

void main() {
    vec3 normal = texture(normalmap, f_tex_coords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    // vec3 normal = normalize(f_normal);

    vec3 color = ub.color * texture(image, f_tex_coords).rgb;

    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);

    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  

    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 ambient = ambientStrength * color;
    vec3 diffuse = diff * color;
    vec3 specular = vec3(0.2) * spec;

    vec3 result = (ambient + diffuse + specular);

    o_color = vec4(result, 1.0);
}