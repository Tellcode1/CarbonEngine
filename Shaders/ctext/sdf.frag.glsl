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

void main() {;
    float dist = 0.5 - texture(bitmaps[0], texCoords).r;
    vec2 ddist = vec2(dFdx(dist), dFdy(dist));
    float pixelDist = dist / length(ddist);
    float alpha = 0.5 - pixelDist;
    outColor = vec4(vec3(alpha), alpha);
}