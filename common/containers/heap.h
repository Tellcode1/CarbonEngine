#ifndef __NOVA_HEAP_H__
#define __NOVA_HEAP_H__

#define NOVA_CHUNK_SIZE (1024)

#include "../stdafx.h"
#include "freelist.h"
#include <sys/mman.h>

#define POOTIS 32686
#define ALIGN_UP(ptr, alignment) (ptr ? ((void*)(((uintptr_t)(ptr) + ((alignment) - 1)) & ~((alignment) - 1))) : ((void*)-1))
#define ALIGN_UP_SIZE(size, align) (((size) + (align) - 1) & ~((align) - 1))

typedef struct NV_heap_node
{
  struct NV_heap_node*  next;
  struct NV_heap_node*  prev;
  struct NV_heap_chunk* parent;
  void*                 payload;
  size_t                size;
  unsigned              canary;
} NV_heap_node;

typedef struct NV_heap_chunk
{
  struct NV_heap_chunk* next;
  struct NV_heap_chunk* prev;
  NV_heap_node*         root;
  uchar*                buf;
  void*                 mapping;
  size_t                mapping_size;
  size_t                offset;
  unsigned              canary;
} NV_heap_chunk;

typedef struct NV_heap
{
  NV_heap_chunk* root;
  unsigned       canary;
} NV_heap;

static inline NV_heap_chunk*
_NV_heap_make_chunk(size_t alignment, size_t size)
{
  NV_assert((alignment & (alignment - 1)) == 0 && alignment > 0);
  NV_assert(size < SIZE_MAX - alignment - sizeof(NV_heap_chunk) - sizeof(unsigned));

  size_t chunk_size = (size < NOVA_CHUNK_SIZE) ? NOVA_CHUNK_SIZE : size;
  chunk_size += sizeof(NV_heap_chunk) + sizeof(NV_heap_node);

  size_t total_size = ALIGN_UP_SIZE(chunk_size, alignment);

  if (total_size <= 0)
  {
    NV_LOG_ERROR("zero size malloc");
    return NULL;
  }

  void* mapping = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (mapping == MAP_FAILED || !mapping)
  {
    NV_LOG_ERROR("mmap failed");
    return NULL;
  }

  NV_heap_chunk* chk = (NV_heap_chunk*)mapping;
  *chk               = (NV_heap_chunk){};
  chk->next          = NULL;
  chk->prev          = NULL;
  chk->root          = NULL;
  chk->buf           = (uchar*)mapping + sizeof(NV_heap_chunk);
  chk->mapping       = mapping;
  chk->mapping_size  = total_size;
  chk->offset        = sizeof(NV_heap_chunk);
  chk->canary        = POOTIS;
  return chk;
}

static inline NV_heap_chunk*
_NV_heap_get_last_chunk(NV_heap* heap)
{
  NV_assert(heap->canary == POOTIS);
  NV_heap_chunk* chunk = heap->root;
  if (!chunk)
  {
    return NULL;
  }
  while (chunk->next)
  {
    chunk = chunk->next;
  }
  return chunk;
}

static inline NV_heap_node*
_NV_heap_chunk_get_last_node(NV_heap_chunk* chunk)
{
  NV_assert(chunk->canary == POOTIS);
  NV_heap_node* node = chunk->root;
  if (!node)
  {
    return NULL;
  }
  while (node->next)
  {
    node = node->next;
  }
  return node;
}

static inline NV_heap_node*
_NV_heap_make_node(NV_heap_chunk* chunk, size_t alignment, size_t size)
{
  NV_assert(chunk->canary == POOTIS);
  size_t aligned_size = ALIGN_UP_SIZE(size, alignment) + sizeof(NV_heap_node);
  size_t available    = chunk->mapping_size - chunk->offset;
  if (available < aligned_size)
  {
    return NULL;
  }

  size_t        offset = chunk->mapping_size - available;
  NV_heap_node* node   = (NV_heap_node*)(chunk->buf + offset);
  *node                = (NV_heap_node){};

  node->payload        = ALIGN_UP(node + 1, alignment);
  node->size           = size;
  node->canary         = POOTIS;
  chunk->offset += aligned_size;

  NV_heap_node* last = _NV_heap_chunk_get_last_node(chunk);
  if (last)
  {
    NV_assert(last->canary == POOTIS);
    last->next = node;
    node->prev = last;
  }
  else
  {
    chunk->root = node;
    node->prev  = NULL;
  }
  node->next   = NULL;
  node->parent = chunk;

  return node;
}

static inline void*
NV_heap_alloc(NV_heap* heap, size_t alignment, size_t size)
{
  NV_assert(heap->canary == POOTIS);

  if (!heap->root)
  {
    NV_heap_chunk* chunk = _NV_heap_make_chunk(alignment, size);
    if (!chunk)
    {
      return NULL;
    }
    NV_heap_node* node = _NV_heap_make_node(chunk, alignment, size);
    if (!node)
    {
      munmap(chunk->mapping, chunk->mapping_size);
      return NULL;
    }
    heap->root  = chunk;
    chunk->root = node;
    return node->payload;
  }

  const size_t   aligned_size = ALIGN_UP_SIZE(size, alignment) + sizeof(NV_heap_node);
  NV_heap_chunk* chunk        = heap->root;
  while (chunk)
  {
    NV_assert(chunk->canary == POOTIS);
    if ((chunk->mapping_size - chunk->offset) >= aligned_size)
    {
      NV_heap_node* node = _NV_heap_make_node(chunk, alignment, size);
      return node ? node->payload : NULL;
    }
    chunk = chunk->next;
  }

  chunk = _NV_heap_make_chunk(alignment, size);
  if (!chunk)
  {
    return NULL;
  }
  NV_heap_chunk* last = _NV_heap_get_last_chunk(heap);
  NV_assert(last->canary == POOTIS);
  if (last)
  {
    last->next  = chunk;
    chunk->prev = last;
  }
  else
  {
    heap->root  = chunk;
    chunk->prev = heap->root;
  }
  NV_heap_node* node = _NV_heap_make_node(chunk, alignment, size);
  return node ? node->payload : NULL;
}

static inline void
NV_heap_free(NV_heap* heap, void* block)
{
  // TODO: Implement freeing logic
}

#endif //__NOVA_HEAP_H__