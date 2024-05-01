#include "DMalloc.hpp"

static bool IsPowerOfTwo(dmu8ptr n) {
	return (n & (n - 1)) == 0;
}

dmu8ptr AlignForward(dmu8ptr ptr, dmu8ptr align) {
	dmu8ptr p, arena, modulo;

	assert(IsPowerOfTwo(align));

	p = ptr;
	arena = static_cast<dmu8ptr>(align);
	modulo = p & (arena - 1);

	if (modulo != 0) {
		// If 'p' address is not aligned, push the address to the
		// next value which is aligned
		p += arena - modulo;
	}
	return p;
}


void dm::DMInit()
{
    // TODO
}

dm::arena::Arena dm::arena::CreateArena(void* backBuffer, u64 length)
{
    Arena arena;
    arena.buffer = reinterpret_cast<uchar*>(backBuffer);
    arena.bufferLength = length;
    arena.previousOffset = 0;
    arena.currentOffset = 0;

    return arena;
}

void dm::arena::DestroyArena(Arena *arena, bool freeMemory)
{
    if (freeMemory)
        free(arena->buffer);
}

void *dm::arena::alloc(Arena *arena, u64 size, u64 align)
{
    assert(arena != nullptr);

    // Align 'curr_offset' forward to the specified alignment
	dmu8ptr currentPtr = reinterpret_cast<dmu8ptr>(arena->buffer) + reinterpret_cast<dmu8ptr>(arena->currentOffset);
	dmu8ptr offset = AlignForward(currentPtr, align);

	offset -= reinterpret_cast<dmu8ptr>(arena->buffer); // Change to relative offset

	// Check to see if the backing memory has space left
	if (offset + size <= arena->bufferLength)
    {
		void* ptr = &arena->buffer[offset];
		arena->previousOffset = offset;
		arena->currentOffset = offset + size;

		return ptr;
	}

	// Return NULL if the arena is out of memory (or handle differently)
	return NULL;
}
