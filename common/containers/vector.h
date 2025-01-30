#ifndef __NOVA_VECTOR_H__
#define __NOVA_VECTOR_H__

#include "../mem.h"
#include <pthread.h>

NOVA_HEADER_START;

extern NVAllocatorHeap pool;
extern bool              init;

typedef struct NV_vector_t
{
  unsigned         m_canary;
  size_t           m_size;
  size_t           m_capacity;
  int              m_typesize;
  void*            m_data;
  pthread_rwlock_t m_rwlock;
  NVAllocator    allocator;
} NV_vector_t;
typedef int (*NV_vector_compare_fn)(const void* obj1, const void* obj2);

/*
    initial_size may be 0
*/
extern NV_vector_t NV_vector_init(int typesize, size_t init_size);
extern void        NV_vector_destroy(NV_vector_t* vec);

extern void        NV_vector_resize(NV_vector_t* vec, size_t new_size);
extern void        NV_vector_clear(NV_vector_t* vec);

extern size_t      NV_vector_size(const NV_vector_t* vec);
extern size_t      NV_vector_capacity(const NV_vector_t* vec);
extern int         NV_vector_typesize(const NV_vector_t* vec);
extern void*       NV_vector_data(const NV_vector_t* vec);

extern void*       NV_vector_back(NV_vector_t* vec);

extern void*       NV_vector_get(const NV_vector_t* vec, size_t i);
extern void        NV_vector_set(NV_vector_t* vec, size_t i, void* elem);

// Overrides contents
extern void NV_vector_copy_from(const NV_vector_t* __restrict src, NV_vector_t* __restrict dst);
// src is destroyed and unusable after this call!
extern void NV_vector_move_from(NV_vector_t* __restrict src, NV_vector_t* __restrict dst);

extern bool NV_vector_empty(const NV_vector_t* vec);

// Doesn't mean that the two vectors ptrs are pointing to the same vector
// This'll check the size and the data only, not the capacity (why would you?)
extern bool NV_vector_equal(const NV_vector_t* vec1, const NV_vector_t* vec2);

// WARNING: sizeof(*elem) != vec->typesize is UNDEFINED!
extern void NV_vector_push_back(NV_vector_t* __restrict vec, const void* __restrict elem);
extern void NV_vector_push_set(NV_vector_t* __restrict vec, const void* __restrict arr, size_t count);

extern void NV_vector_pop_back(NV_vector_t* vec);
extern void NV_vector_pop_front(NV_vector_t* vec); // expensive

extern void NV_vector_insert(NV_vector_t* __restrict vec, size_t index, const void* __restrict elem);
extern void NV_vector_remove(NV_vector_t* vec, size_t index);

extern int  NV_vector_find(const NV_vector_t* __restrict vec, const void* __restrict elem);

extern void NV_vector_sort(NV_vector_t* vec, NV_vector_compare_fn compare);

NOVA_HEADER_END;

#endif //__NOVA_VECTOR_H__