#version 450

#define SUPERSAMPLE

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

const uint mode = SDF;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

// This file should be seperated into 3, one for sdf, msdf and mtsdf which will be loaded at
// runtime if needed. We can stick to one for debugging.

const float pxRange = 32.0f;
float screen_px_range() {
    vec2 unit_range = vec2(pxRange)/vec2(textureSize(bitmaps[0], 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(texCoords);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

float contour(in float d, in float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}

float samp(in vec2 uv, float w) {
    return contour(texture(bitmaps[0], uv).a, w);
}

void main() {;
    if (mode == SDF) {
        float dist = 0.5 - texture(bitmaps[0], texCoords).r;
        vec2 ddist = vec2(dFdx(dist), dFdy(dist));
        float pixelDist = dist / length(ddist);
        float alpha = 0.5 - pixelDist;
        outColor = vec4(vec3(alpha), alpha);
    }
    else if (mode == MSDF) {
        vec3 distance = texture(bitmaps[0], texCoords).rgb;
        float dist = median(distance.r, distance.g, distance.b);
        float pxDist = screen_px_range() * (dist - 0.5);
        float opacity = clamp(pxDist + 0.5, 0.0, 1.0);
        outColor = vec4(1.0, 1.0, 1.0, opacity);
    } else if (mode == MTSDF) {
        outColor = texture(bitmaps[0], texCoords);
    }
}