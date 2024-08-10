#version 450

layout(location=0) out
vec4 outColor;

layout(location=0) in
vec2 texCoords;

layout (location=1) in flat
float scale;// pxRange * scale

layout(set=0,binding=0) uniform
sampler2D bitmap;

float screen_px_range() {
    vec2 unit_range = vec2(scale)/vec2(textureSize(bitmap, 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(texCoords);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

void main() {
    float contour_width = (sqrt(0.5)) / screen_px_range(); // u can adjust this a little
    float distance = texture(bitmap, texCoords).r;
    float alpha = smoothstep(0.5 - contour_width, 0.5 + contour_width, distance);
    outColor = vec4(vec3(1.0), alpha);
}