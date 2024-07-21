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

const uint mode = MSDF;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

// This file should be seperated into 3, one for sdf, msdf and mtsdf which will be loaded at
// runtime if needed. We can stick to one for debugging.

float screenPxRange(sampler2D tex) {
	vec2 unitRange = vec2(6.0) / vec2(textureSize(tex, 0));
	vec2 screenTexSize = vec2(1.0) / fwidth(texCoords);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main() {;
    if (mode == SDF) {
        float distance = texture(bitmaps[0], texCoords).r;
        float smoothWidth = fwidth(distance);
        float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
        outColor = vec4(vec3(alpha), 1.0);
    }
    else {
        vec3 msd = texture(bitmaps[0], texCoords).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = 4.5*(sd - 0.5);
        float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
        const vec4 bgColor = vec4(0.0, 0.0, 0.0, 1.0);
        const vec4 fgColor = vec4(1.0, 1.0, 1.0, 1.0);
        outColor = mix(bgColor, fgColor, opacity);
    }
}