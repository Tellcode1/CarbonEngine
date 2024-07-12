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

const uint SDF = 1;
const uint MSDF = 3;
const uint MTSDF = 4;

const uint mode = MTSDF;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {;
    if (mode == SDF)
        outColor = vec4(vec3(1.0, 1.0, 1.0) * texture(bitmaps[0], texCoords).r, 1.0);
    else {
        outColor = texture(bitmaps[0], texCoords).rgba;
    }
}