// output: Shaders/ctext/msdf.frag.spv stage: frag name: ctext/msdf

#version 450

layout(location=0) out
vec4 o_color;

layout(location=0) in
vec2 f_uv;

layout (location=1) in flat
float pxRange;// pxRange * scale

layout(set=0,binding=0) uniform
sampler2D bitmap;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screen_px_range() {
    vec2 unit_range = vec2(pxRange) / vec2(textureSize(bitmap, 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(f_uv);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

float contour(in float d, in float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}

void main() {
    // Request the devil for cleaner text
    vec3 distance = texture(bitmap, f_uv).rgb;
    float dist = median(distance.r, distance.g, distance.b);
    float contour_width = (0.666) / screen_px_range(); // u can adjust this a little
    float alpha = contour(dist, contour_width);
    if (alpha <= 0.0)
        discard;
    o_color = vec4(vec3(1.0), alpha);
}