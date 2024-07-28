#version 450

const uint MAX_FONT_COUNT = 8;

layout(location=0) out
vec4 outColor;

layout(location=1) in flat
uint texture_index;

layout(location=0) in
vec2 texCoords;

layout(set=0,binding=0) uniform
sampler2D bitmaps[ MAX_FONT_COUNT ];

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

const float pxRange = 32.0f;
float screen_px_range() {
    vec2 unit_range = vec2(pxRange)/vec2(textureSize(bitmaps[0], 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(texCoords);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

float contour(in float d, in float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}

void main() {
    vec3 distance = texture(bitmaps[texture_index], texCoords).rgb;
    float dist = median(distance.r, distance.g, distance.b);
    float pxDist = screen_px_range() * (dist - 0.5);
    float opacity = clamp(pxDist + 0.5, 0.0, 1.0);
    outColor = vec4(1.0, 1.0, 1.0, opacity);
}