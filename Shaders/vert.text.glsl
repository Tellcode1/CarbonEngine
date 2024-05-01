#version 450 core

layout(location=0) in
vec4 verticesAndUV; // <vec2 vertices, vec2 uv>

layout(location=0) out
vec2 FragTexCoord;

layout(push_constant) uniform PushConstantData {
    mat4 model;
};

void main() {
    gl_Position = model * vec4(verticesAndUV.xy, 0.0, 1.0);
    FragTexCoord = verticesAndUV.zw;
}