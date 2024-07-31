#version 450

layout(location=0) in
vec4 verticesAndTexCoords;

layout(push_constant) uniform push_constant
{
    mat4 pc_model;
} pc;

layout(location=0) out
vec2 texCoords;

void main() {
    gl_Position = pc.pc_model * vec4(verticesAndTexCoords.xy, 0.0, 1.0);
    texCoords = verticesAndTexCoords.zw;
}
