// output: Shaders/ctext/text.spv stage: frag name: ctext/frag

#version 450

layout(location=0) out
vec4 outColor;

layout(location=0) in
vec2 texCoords;

layout(location=1) in
vec4 f_col;

layout(set=1,binding=0) uniform
sampler2D bitmap;

layout (push_constant) uniform push_constants {
    mat4 model;
    vec4 color;
    vec4 outline_color;
    float scale;
} pc;

float screen_px_range() {
    vec2 unit_range = vec2(pc.scale)/vec2(textureSize(bitmap, 0));
    vec2 screen_tex_size = vec2(1.0)/fwidth(texCoords);
    return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

void main() {
    float contour_width = 0.2 / screen_px_range(); // u can adjust this a little
    float distance = texture(bitmap, texCoords).r;
    float alpha = smoothstep(0.5 - contour_width, 0.5 + contour_width, distance);
    if (alpha <= 0.01)
        discard;
    outColor = vec4(vec3(1.0), alpha);
}