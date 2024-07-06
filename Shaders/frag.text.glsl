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
    outColor = texture(bitmaps[0], texCoords);
    // const vec2 uv = gl_FragCoord.xy / vec2(660.0, 730.0);
    // outColor = vec4(uv.x, 1.0 - uv.y, 0.0, 1.0);
}