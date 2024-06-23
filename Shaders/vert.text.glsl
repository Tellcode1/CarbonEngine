#version 450 core

layout(location=0) in
vec4 verticesAndUV; 

layout(location=0) out
vec2 FragTexCoord;

void main() {
    gl_Position = vec4(verticesAndUV.xy, 0.0, 1.0);
    FragTexCoord = verticesAndUV.zw;
}