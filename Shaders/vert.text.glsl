#version 450

layout(location=0) in
vec4 verticesAndTexCoords;

layout(push_constant) uniform push_constant
{
    mat4 model;
} pc;

layout(location=0) out
vec2 texCoords;

layout(location=1) out flat
uint textureIndex;

void main() {
    gl_Position = pc.model * vec4(verticesAndTexCoords.xy, 0.0, 1.0);
    texCoords = verticesAndTexCoords.zw;
    textureIndex = 0; // REPLACE
}
