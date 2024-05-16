#version 450 core

layout(location=0) in
vec4 verticesAndUV;

layout(location=0) out
vec2 FragTexCoord;

layout(location=1) flat out
vec2 outTextureIndexAndScale;

layout(push_constant)
uniform PC {
    mat2 mvp;
    uint textureIndex;
    float scale;
};

void main() {
    gl_Position = vec4(mvp * verticesAndUV.xy, 0.0, 1.0);
    FragTexCoord = verticesAndUV.zw;
    outTextureIndexAndScale = vec2(textureIndex, scale);
}