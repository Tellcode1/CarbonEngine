#version 450 core

layout(constant_id=0) const
uint MAX_FONT_COUNT = 1;

layout(location=0) out
vec4 FragColor;

layout(location=0) in
vec2 FragTexCoord;

layout(location=1) in flat
uint textureIndex;

layout(set=0, binding=0) uniform
sampler2D fontSampler[ MAX_FONT_COUNT ];

void main() {
    const float dist = texture(fontSampler[textureIndex], FragTexCoord).r;
    FragColor = vec4(vec3(1.0, 1.0, 1.0), dist);
}
