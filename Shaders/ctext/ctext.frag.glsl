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

void main() {
    float dist = texture(bitmap, texCoords).r;
    const float smoothing = 0.1 / pc.scale;
    float alpha = smoothstep(pc.scale - smoothing, pc.scale + smoothing, dist);
    outColor = vec4(f_col.rgb, f_col.a * alpha);
}