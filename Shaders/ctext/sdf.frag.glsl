#version 450

layout(location=0) out
vec4 outColor;

layout(location=0) in
vec2 texCoords;

layout(set=0,binding=0) uniform
sampler2D bitmap;

void main() {;
    float dist = 0.5 - texture(bitmap, texCoords).r;
    if (dist == 0.0)
        discard;
    vec2 ddist = vec2(dFdx(dist), dFdy(dist));
    float pixelDist = dist / length(ddist);
    float alpha = 0.5 - pixelDist;
    outColor = vec4(vec3(alpha), alpha);
}