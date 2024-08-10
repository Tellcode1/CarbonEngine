#version 450

layout(location=0) out
vec4 outColor;

layout(location=0) in
vec2 texCoords;

layout (location=1) in flat
float scale;// pxRange * scale

layout(set=0,binding=0) uniform
sampler2D bitmap;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screen_px_range() {
    vec2 unit_range = vec2(scale)/vec2(textureSize(bitmap, 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(texCoords);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

float contour(in float d, in float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}

void main() {
    vec4 samp = texture(bitmap, texCoords).rgba;
    float msdf_dist = median(samp.r, samp.g, samp.b);    
    float distance = mix(msdf_dist, samp.a, 0.5);

    float contour_width = (0.7) / screen_px_range(); // u can adjust this a little
    float alpha = contour(distance, contour_width);

    outColor = vec4(vec3(1.0), alpha);
}