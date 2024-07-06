#version 450

const uint MAX_FONT_COUNT = 8;

layout(location=0) out
vec4 outColor;

layout(location=1) in flat
uint textureIndex;

layout(location=0) in
vec2 texCoords;

layout(set=0,binding=0) uniform
sampler2D bitmaps[ MAX_FONT_COUNT ];

void main() {
    // outColor = texture(bitmaps[0], texCoords);
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}