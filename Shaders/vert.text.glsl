#version 450

layout(location=0) in
vec4 verticesAndTexCoords;

layout(push_constant) uniform push_constant
{
    mat4 pc_model;
    float pc_scale;
} pc;

layout(location=0) out
vec2 texCoords;

layout (location=1) out flat
float scale;// pxRange * scale

void main() {
    gl_Position = pc.pc_model * vec4(verticesAndTexCoords.xy, 0.0, 1.0);
    texCoords = verticesAndTexCoords.zw;
    scale = pc.pc_scale;
}
