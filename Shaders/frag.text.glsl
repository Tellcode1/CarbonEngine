#version 450 core

layout(location=0) out
vec4 FragColor;

layout(location=0) in
vec2 FragTexCoord;

// layout(set=0, binding=0) uniform
// sampler2D fontSampler;

void main() {
    FragColor = vec4(vec3(1.0, 1.0, 1.0), 1.0);
}
