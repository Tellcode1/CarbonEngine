#version 450 core

layout(location=0) in
vec4 verticesAndTexCoord; // <vec2 vertices, vec2 texCoord>

layout(location=0) out
vec2 FragTexCoord;

layout(push_constant) uniform PushConstantData {
    mat4 model;
};

void main() {
    gl_Position = model * vec4(verticesAndTexCoord.xy, 0.0, 1.0);
    FragTexCoord = verticesAndTexCoord.zw;
}