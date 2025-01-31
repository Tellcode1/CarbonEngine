#ifndef __NOVA_FREELIST_H__
#define __NOVA_FREELIST_H__

#include <stdbool.h>
#include <stdlib.h>

#include "../stdafx.h"
#include "../string.h"

typedef struct NV_chunk_t    NV_chunk_t;
typedef struct NV_node_t     NV_node_t;
typedef struct NV_freelist_t NV_freelist_t;

typedef NV_node_t* (*NV_freelist_alloc_fn)(size_t alignment, size_t size); // allocate the payload and set the size
typedef void (*NV_freelist_free_fn)(NV_node_t* node);                      // you need to free the node's data and the node!!!

struct NV_chunk_t
{
  void* mapping;
  // available is here for easy querying by the freelist
  size_t     mapping_size, mapping_offset, available;
  NV_node_t* root;
};

struct NV_node_t
{
  struct NV_node_t* next;
  NV_chunk_t*       chunk;
  void*             payload;
  void*             mapping;
  size_t            mapping_size;
  size_t            size; // allocated size
  unsigned          canary;
  bool              in_use;
};

struct NV_freelist_t
{
  unsigned             m_canary;
  NV_node_t*           m_root;
  NV_freelist_alloc_fn m_alloc_fn;
  NV_freelist_free_fn  m_free_fn;
  pthread_rwlock_t     m_rwlock;
};

extern NV_chunk_t* NV_freelist_make_chunk(const NV_freelist_t* list, size_t alignment, size_t size);
extern NV_node_t*  NV_freelist_mknode(const NV_freelist_t* list, size_t alignment, size_t size);

// Initialize a freelist
extern void NV_freelist_init(size_t init_size, NV_freelist_alloc_fn alloc_fn, NV_freelist_free_fn free_fn, NV_freelist_t* list);

extern void NV_freelist_destroy(NV_freelist_t* list);

// Allocate memory from the list. Will allocate a new node if there is no space left!!!
extern void* NV_freelist_alloc(NV_freelist_t* list, size_t alignment, size_t size);

// Add a new node with guaranteed extra space.
extern NV_node_t* NV_freelist_expand(NV_freelist_t* list, size_t alignment, size_t expand_by);

// frees the block and its node
// however, if the node is surrounded by free data, it will be coalesced together
extern void NV_freelist_free(NV_freelist_t* list, void* block);

// find the node that owns the block
extern NV_node_t* NV_freelist_find(NV_freelist_t* list, void* alloc);

extern void       NV_freelist_check_circle(const NV_freelist_t* list);

#endif //__NOVA_FREELIST_H__