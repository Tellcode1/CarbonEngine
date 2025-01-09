#ifndef __LUNA_MEM_H__
#define __LUNA_MEM_H__

#include "stdafx.h"

/// @param dstsz The size of the destination
/// @param sz The number of bytes to copy
/// @return Returns NULL on error and dst on success
extern void *luna_memcpy(void *dst, size_t dstsz, const void *src, size_t sz);

/// @brief Sets 'sz' bytes of 'dst' to 'to'
/// @return Returns NULL on error and dst for success
extern void *luna_memset(void *dst, ubyte to, size_t sz);

extern void *luna_memmove(void *dst, size_t dstsz, const void *src, size_t sz);

// i don't want to write my own allocator.
// pls leave me alone
// does basic error checking
extern void *luna_malloc(size_t sz);

extern int luna_compress_data(const void *input, size_t input_size, void *output, size_t *output_size);

extern int luna_decompress_data(const void *compressed_data, size_t compressed_size, void *o_buf, size_t o_buf_sz);

#endif//__LUNA_MEM_H__