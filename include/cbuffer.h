#ifndef __CGFXBUFFER_H
#define __CGFXBUFFER_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum cgfx_buffer_type {
    CGFX_BUFFER_TYPE_VERTEX_BUFFER = 0,
    CGFX_BUFFER_TYPE_INDEX_BUFFER = 1,
    CGFX_BUFFER_TYPE_SSBO = 2,
} cgfx_buffer_type;

// I feel like there are a lot of meme words that start with 'P' and end with 'S' for some reason
typedef enum cgfx_buffer_usage {
    CGFX_PIPIS = 0,
    CGFX_GOOBER = 1,
    CGFX_POOTIS = 2,
} cgfx_buffer_usage;

typedef struct cbuffer_t cbuffer_t;

cbuffer_t *cbuffer_create(int size, cgfx_buffer_type type, cgfx_buffer_usage usage); // and a memory handle?
void cbuffer_bind(cbuffer_t *buf);

#ifdef __cplusplus
    }
#endif

#endif//__CGFXBUFFER_H