#ifndef __NOVA_VECTOR_H__
#define __NOVA_VECTOR_H__

#include "../mem.h"
#include "heap.h"
#include <pthread.h>

NOVA_HEADER_START;

extern NV_heap heap;
extern bool            init;

typedef struct NV_dynarray_t
{
  unsigned         m_canary;
  size_t           m_size;
  size_t           m_capacity;
  int              m_typesize;
  void*            m_data;
  pthread_rwlock_t m_rwlock;
  NVAllocator      allocator;
} NV_dynarray_t;
typedef int (*NV_dynarray_compare_fn)(const void* obj1, const void* obj2);

/*
    initial_size may be 0
*/
extern NV_dynarray_t NV_dynarray_init(int typesize, size_t init_size);
extern void          NV_dynarray_destroy(NV_dynarray_t* vec);

extern void          NV_dynarray_resize(NV_dynarray_t* vec, size_t new_size);
extern void          NV_dynarray_clear(NV_dynarray_t* vec);

extern size_t        NV_dynarray_size(const NV_dynarray_t* vec);
extern size_t        NV_dynarray_capacity(const NV_dynarray_t* vec);
extern int           NV_dynarray_typesize(const NV_dynarray_t* vec);
extern void*         NV_dynarray_data(const NV_dynarray_t* vec);

extern void*         NV_dynarray_back(NV_dynarray_t* vec);

extern void*         NV_dynarray_get(const NV_dynarray_t* vec, size_t i);
extern void          NV_dynarray_set(NV_dynarray_t* vec, size_t i, void* elem);

// Overrides contents
extern void NV_dynarray_copy_from(const NV_dynarray_t* __restrict src, NV_dynarray_t* __restrict dst);
// src is destroyed and unusable after this call!
extern void NV_dynarray_move_from(NV_dynarray_t* __restrict src, NV_dynarray_t* __restrict dst);

extern bool NV_dynarray_empty(const NV_dynarray_t* vec);

// Doesn't mean that the two vectors ptrs are pointing to the same vector
// This'll check the size and the data only, not the capacity (why would you?)
extern bool NV_dynarray_equal(const NV_dynarray_t* vec1, const NV_dynarray_t* vec2);

// WARNING: sizeof(*elem) != vec->typesize is UNDEFINED!
extern void NV_dynarray_push_back(NV_dynarray_t* __restrict vec, const void* __restrict elem);
extern void NV_dynarray_push_set(NV_dynarray_t* __restrict vec, const void* __restrict arr, size_t count);

extern void NV_dynarray_pop_back(NV_dynarray_t* vec);
extern void NV_dynarray_pop_front(NV_dynarray_t* vec); // expensive

extern void NV_dynarray_insert(NV_dynarray_t* __restrict vec, size_t index, const void* __restrict elem);
extern void NV_dynarray_remove(NV_dynarray_t* vec, size_t index);

extern int  NV_dynarray_find(const NV_dynarray_t* __restrict vec, const void* __restrict elem);

extern void NV_dynarray_sort(NV_dynarray_t* vec, NV_dynarray_compare_fn compare);

NOVA_HEADER_END;

#endif //__NOVA_VECTOR_H__