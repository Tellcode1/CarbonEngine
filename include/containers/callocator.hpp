#ifndef __C_ALLOCATOR_H
#define __C_ALLOCATOR_H

#include <cstdlib>
#include "../math/math.h"

typedef void *(*callocator_allocate_fn)(int size, int tpsize, int capacity);
typedef void (*callocator_free_fn)(void *ptr);

template <typename tp, callocator_allocate_fn alloc, callocator_free_fn free>
struct callocator {
    void *memory;
    int size, capacity;

    
};

#endif//__C_ALLOCATOR_H