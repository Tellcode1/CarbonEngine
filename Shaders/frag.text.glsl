#version 450 core

layout(location = 0) out
vec4 FragColor;

layout(location=0) in
vec2 FragTexCoord;

layout(location=1) flat in
vec2 outTextureIndexAndScale;

layout(set=0, binding=0) uniform
sampler fontSampler;

layout(set=0, binding=1) uniform
texture2D fontTextures[8];

void main() {
    // const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(sampler2D(fontTextures[outTextureIndex], fontSampler), FragTexCoord.xy).r);
    const uint textureIndex = uint(outTextureIndexAndScale.x);
    const float smoothing = clamp(0.1, 0.5, outTextureIndexAndScale.y);

    const float distance = texture(sampler2D(fontTextures[textureIndex], fontSampler), FragTexCoord.xy).r;
    const float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    
    FragColor = vec4(vec3(1.0, 1.0, 1.0), alpha);
    // FragColor = vec4(vec3(1.0, 1.0, 1.0), 1.0);
}
