#ifndef __LUNA_STR_H__
#define __LUNA_STR_H__

#include <stddef.h>
#include <stdint.h>

// This is literally using the 'n' types of the string functions

/// @param dstsz The size of the destination
/// @param sz The number of bytes to copy
/// @return Returns NULL on error and dst on success
extern void *luna_memcpy(void *dst, size_t dstsz, const void *src, size_t sz);

/// @brief Sets 'sz' bytes of 'dst' to 'to'
/// @return Returns NULL on error and dst for success
extern void *luna_memset(void *dst, char to, size_t sz);

extern void *luna_memmove(void *dst, size_t dstsz, const void *src, size_t sz);

extern void *luna_memchr(const void *p, int chr, size_t psize);

// i don't want to write my own allocator.
// pls leave me alone
// does basic error checking
extern void *luna_malloc(size_t sz);

extern void luna_free(void *block);

// Uses zlib to compress and decompress the image
extern int luna_bufcompress(const void *input, size_t input_size, void *output, size_t *output_size);

extern int luna_bufdecompress(const void *compressed_data, size_t compressed_size, void *o_buf, size_t o_buf_sz);

extern size_t luna_strlen(const char *s);

extern size_t luna_strncpy(char *dest, const char *src, size_t max);

extern int luna_strncmp(const char *s1, const char *s2, size_t max);

extern char *luna_strchr(char *str, int chr);

extern char *luna_strrchr(char *str, int chr);

static inline size_t luna_strcpy(char *dest, const char *src) {
  return luna_strncpy(dest, src, SIZE_MAX);
}

static inline int luna_strcmp(const char *s1, const char *s2) {
  return luna_strncmp(s1, s2, luna_strlen(s1));
}

#endif //__LUNA_STR_H__