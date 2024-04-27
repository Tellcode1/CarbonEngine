#version 450 core

struct InBuffer
{
    unsigned char character;
};

struct OutBuffer
{
    vec4 positionAndTextureCoords;
};