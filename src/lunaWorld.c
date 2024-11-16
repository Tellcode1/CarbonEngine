#include "../include/lunaWorld.h"
#include "../include/vkstdafx.h"
#include "../include/math/vec2.h"
#include "../include/math/vec3.h"
#include "lunaWorld.h"

static const int CHUNK_X = 16, CHUNK_Z = 16, CHUNK_Y = 256;

typedef struct block_vertex {
    vec3 position;
    vec2 texture_coordinates;
} block_vertex;

static const block_vertex block_vertices[4] = {
    (block_vertex){ { -0.5f, -0.5f,  1.0f }, { 0.0f, 0.0f } },
    (block_vertex){ {  0.5f, -0.5f,  1.0f }, { 1.0f, 0.0f } },
    (block_vertex){ {  0.5f,  0.5f,  1.0f }, { 1.0f, 1.0f } },
    (block_vertex){ { -0.5f,  0.5f,  1.0f }, { 0.0f, 1.0f } },
};
static const u32 block_indices[6] = {
    0, 1, 2,   2, 3, 0,
};

typedef struct luna_Block_t {
    float gridx, gridy, gridz;
} luna_Block_t;

static const int total_vertices_size = sizeof(block_vertices);
static const int total_indices_size = sizeof(block_indices);

typedef struct luna_Chunk_t {
    luna_GPU_Buffer vertex_buffer;
    luna_GPU_Buffer index_buffer;
    luna_GPU_Memory memory;
    luna_Block_t blocks[ CHUNK_X * CHUNK_Y * CHUNK_Z ];
} luna_Chunk_t;

typedef struct luna_World_t {
    // 2 * 2 world for testing
    luna_Chunk_t chunks[ 2 * 2 ];
} luna_World_t;

void luna_chunk_init(const luna_World_t *world, luna_Chunk_t *chunk) {
    luna_GPU_AllocateMemory(
        total_vertices_size + total_indices_size,
        LUNA_GPU_MEMORY_USAGE_CPU_WRITEABLE | LUNA_GPU_MEMORY_USAGE_GPU_LOCAL,
        &chunk->memory
    );

    luna_GPU_CreateBuffer(
        total_vertices_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &chunk->vertex_buffer
    );

    luna_GPU_CreateBuffer(
        total_indices_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &chunk->index_buffer
    );

    luna_GPU_BindBufferToMemory(&chunk->memory, 0, &chunk->vertex_buffer);
    luna_GPU_BindBufferToMemory(&chunk->memory, total_vertices_size, &chunk->index_buffer);

    for (int x = 0; x < CHUNK_X; x++) {
        for (int y = 0; y < CHUNK_Y; y++) {
            for (int z = 0; z < CHUNK_Z; z++) {
                luna_Block_t *block = &chunk->blocks[ x + (y * CHUNK_X) + (z * CHUNK_X * CHUNK_Y) ];
                block->gridx = x;
                block->gridy = y;
                block->gridz = z;
            }
        }
    }
}

luna_World_t *luna_world_init() {
}

void luna_world_render(luna_World_t *world)
{
    
}