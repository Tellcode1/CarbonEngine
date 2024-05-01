#ifndef __DMALLOC_HPP__
#define __DMALLOC_HPP__

#include "stdafx.hpp"

// Under heavy development (by one person)

/*
*   TODO
*/
enum DMInitFlags : u32
{
    DM_INIT_ARENA_ONLY = 0x00000001
};

typedef uintptr_t dmu8ptr;

namespace dm
{
    /*
    *   Initialize free list, etc.
    */
    void DMInit();

    namespace arena
    {
        struct Arena
        {
            uchar* buffer;
            u64 bufferLength;
            u64 previousOffset;
            u64 currentOffset;
            
        };

        Arena CreateArena(void* backBuffer, u64 length);
        void DestroyArena(Arena* arena, bool freeMemory = false);
        void* alloc(Arena* arena, u64 size, u64 align = 2 * sizeof(dmu8ptr));
    }
    
    // static bool __dm_was_init = false;
}

#endif