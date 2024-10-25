// output: Shaders/ctext/text.spv stage: frag name: ctext/frag

#version 450

layout(location=0) out
vec4 outColor;

layout(location=0) in
vec2 texCoords;

layout(location=1) in
vec4 f_col;

layout(set=0,binding=0) uniform
sampler2D bitmap;

void main() {
    float distance = texture(bitmap, texCoords).r;
    if (distance < 0.01) {
        discard;
    }
    outColor = vec4(vec3(1.0), distance);
}