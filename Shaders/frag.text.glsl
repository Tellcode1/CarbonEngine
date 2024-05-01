#version 450 core

layout(location = 0) out vec4 FragColor;

layout(location=0) 
in vec2 FragTexCoord;

layout(binding = 1)
uniform sampler2D fontTexture;

void main() {
    const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(fontTexture, FragTexCoord.xy).r);
    FragColor = sampled;
}
