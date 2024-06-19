#version 450 core

layout(location=0) in
vec4 verticesAndUV; 

layout(location=0) out
vec2 FragTexCoord;

layout(location=1) out flat
uint textureIndex;

layout(push_constant)
uniform PC
{
    mat4 mvp;
    uint inTextureIndex;
};

void main() {
    gl_Position = mvp * vec4(verticesAndUV.xy, 0.0, 1.0);
    FragTexCoord = verticesAndUV.zw;
    textureIndex = inTextureIndex;
}