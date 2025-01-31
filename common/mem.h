#ifndef __NOVA_MEM_H__
#define __NOVA_MEM_H__

#include "../common/containers/freelist.h"
#include "../common/containers/heap.h"
#include "stdafx.h"
#include "string.h"
#include <stddef.h>

NOVA_HEADER_START;

#define NOVA_ALLOCATION_CANARY (0xBEEFDEAD)

// The smallest size of memory that the allocator will take from a page
// i.e. The smallest amount of memory handled by lmalloc internally, this chunk will be broken down into subchunks and returned.
#ifndef LMALLOC_DEFAULT_CHUNK_SIZE
#define LMALLOC_DEFAULT_CHUNK_SIZE 512
#endif

#ifndef LMALLOC_DEFAULT_PAGE_SIZE
#define LMALLOC_DEFAULT_PAGE_SIZE 4096
#endif

typedef struct NVAllocator      NVAllocator;
typedef struct NVAllocatorStack NVAllocatorStack;
typedef struct NVAllocatorHeap  NVAllocatorHeap;

// Stack Allocator Functions.
extern void  NVAllocatorStackInit(NVAllocatorStack* allocator, unsigned char* buf, size_t available);

extern void* saalloc(NVAllocator* parent, size_t alignment, size_t size);
extern void* sacalloc(NVAllocator* parent, size_t alignment, size_t size);
extern void* sarealloc(NVAllocator* parent, void* prevblock, size_t alignment, size_t size);
extern void  safree(NVAllocator* parent, void* block);

// malloc, calloc, realloc, free
extern void* heapalloc(NVAllocator* parent, size_t alignment, size_t size);
extern void* heapcalloc(NVAllocator* parent, size_t alignment, size_t size);
extern void* heaprealloc(NVAllocator* parent, void* prevblock, size_t alignment, size_t size);
extern void  heapfree(NVAllocator* parent, void* block);

// Custom mmap based heap allocator.
extern void  NVAllocatorHeapInit(NVAllocatorHeap* pool);

extern void* poolmalloc(NVAllocator* allocator, size_t alignment, size_t size);
extern void* poolcalloc(NVAllocator* allocator, size_t alignment, size_t size);
extern void* poolrealloc(NVAllocator* allocator, void* prevblock, size_t alignment, size_t size);
extern void  poolfree(NVAllocator* allocator, void* block);

typedef void* (*NVAllocatorAllocFn)(NVAllocator* allocator, size_t alignment, size_t size);
typedef NVAllocatorAllocFn NVAllocatorCallocFn;
typedef void* (*NVAllocatorReallocFn)(NVAllocator* allocator, void* prevblock, size_t alignment, size_t size);
typedef void (*NVAllocatorFreeFn)(NVAllocator* allocator, void* block);

struct NVAllocator
{
  NVAllocatorAllocFn   alloc;
  NVAllocatorCallocFn  calloc;
  NVAllocatorReallocFn realloc;
  NVAllocatorFreeFn    free;
  void*                context;
  void*                user_data;
};

// malloc, realloc, free
extern NVAllocator NVAllocatorDefault;

struct NVAllocatorStack
{
  unsigned char* buf;
  size_t         bufsiz;
  size_t         bufoffset;
};

struct NVAllocatorHeap
{
  NV_freelist_t freelist;
};

static inline void
NVAllocatorBindStackAllocator(NVAllocator* allocator, NVAllocatorStack* stack)
{
  allocator->alloc   = saalloc;
  allocator->calloc  = sacalloc;
  allocator->realloc = sarealloc;
  allocator->free    = safree;
  allocator->context = stack;
}

static inline void
NVAllocatorBindHeapAllocator(NVAllocator* allocator, NV_heap* heap)
{
  allocator->alloc   = poolmalloc;
  allocator->calloc  = poolcalloc;
  allocator->realloc = poolrealloc;
  allocator->free    = poolfree;
  allocator->context = heap;
}

NOVA_HEADER_END;

#endif