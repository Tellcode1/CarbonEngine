#ifndef __LUNA_MEM_H__
#define __LUNA_MEM_H__

#include "stdafx.h"
#include "string.h"
#include <stddef.h>

#define SABLOCKCANARY (0xDEADBEEF)

typedef struct lunaAllocator lunaAllocator;
typedef struct lunaStackAllocator lunaStackAllocator;

// Stack Allocator Functions.
extern void sainit(lunaStackAllocator *allocator, unsigned char *buf, size_t available);

extern void *saalloc(lunaAllocator *parent, size_t size);
extern void *sacalloc(lunaAllocator *parent, size_t size);
extern void *sarealloc(lunaAllocator *parent, void *prevblock, size_t size);
extern void safree(lunaAllocator *parent, void *block);

extern void *heapalloc(lunaAllocator *parent, size_t size);
extern void *heapcalloc(lunaAllocator *parent, size_t size);
extern void *heaprealloc(lunaAllocator *parent, void *prevblock, size_t size);
extern void heapfree(lunaAllocator *parent, void *block);

typedef void *(*lunaAllocatorAllocFn)(lunaAllocator *allocator, size_t size);
typedef lunaAllocatorAllocFn lunaAllocatorCallocFn;
typedef void *(*lunaAllocatorReallocFn)(lunaAllocator *allocator, void *prevblock, size_t size);
typedef void (*lunaAllocatorFreeFn)(lunaAllocator *allocator, void *block);

struct lunaAllocator {
  lunaAllocatorAllocFn alloc;
  lunaAllocatorCallocFn calloc;
  lunaAllocatorReallocFn realloc;
  lunaAllocatorFreeFn free;
  void *context;
  void *user_data;
};

// malloc, realloc, free
extern lunaAllocator lunaAllocatorDefault;

struct lunaStackAllocator {
  unsigned char *buf;
  size_t bufsiz;
  size_t bufoffset;
};

static inline void lunaAllocatorBindStackAllocator(lunaAllocator *allocator, lunaStackAllocator *stack) {
  allocator->alloc   = saalloc;
  allocator->calloc   = sacalloc;
  allocator->realloc = sarealloc;
  allocator->free    = safree;
  allocator->context = stack;
}

#endif