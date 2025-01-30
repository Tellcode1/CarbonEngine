#ifndef __NOVA_STR_H__
#define __NOVA_STR_H__

#include "stdafx.h"
#include <stddef.h>
#include <stdint.h>

NOVA_HEADER_START;

// Whether to use the __builtin functions provided by GCC
// They are generally faster, so no reason not to?
#ifndef __NOVA_STR_USE_BUILTIN
#define __NOVA_STR_USE_BUILTIN 1
#endif

/// @param sz The number of bytes to copy
/// @return Returns NULL on error and dst on success
extern void *NV_memcpy(void *dst, const void *src, size_t sz);

/// @brief Sets 'sz' bytes of 'dst' to 'to'
/// @return Returns NULL on error and dst for success
extern void *NV_memset(void *dst, char to, size_t sz);

extern void *NV_memmove(void *dst, const void *src, size_t sz);

extern void *NV_memchr(const void *p, int chr, size_t psize);

extern int NV_memcmp(const void *p1, const void *p2, size_t max);

// i don't want to write my own allocator.
// pls leave me alone
// does basic error checking
extern void *NV_malloc(size_t sz);

extern void NV_free(void *block);

// Uses zlib to compress and decompress the buffer
// this works just as you'd expect on images
// output should be an allocation of output_size (or bigger)
extern int NV_bufcompress(const void *input, size_t input_size, void *output, size_t *output_size);

extern int NV_bufdecompress(const void *compressed_data, size_t compressed_size, void *o_buf, size_t o_buf_sz);

extern size_t NV_strlen(const char *s);

extern char *NV_strcpy(char *dest, const char *src);

extern char *NV_strncpy(char *dest, const char *src, size_t max);

// Return the number of characters copied.
extern size_t NV_strncpy2(char *dest, const char *src, size_t max);

// Compare two strings, stopping at either s1 or s2's null terminator or at max.
// It will stop when it reaches the null terminator, no segv
extern int NV_strncmp(const char *s1, const char *s2, size_t max);

// Find the first occurence of a character in a string
extern char *NV_strchr(const char *s, int chr);

// Find the last occurence of a character in a string
// Use NV_strstr if you want to find earliest occurence of a string in a string
extern char *NV_strrchr(const char *s, int chr);

// strchr() but string
// Find the earlier occurence of a (sub)string in a string.
// eg. For s Baller and sub ll
// returns a pointer to the first l
extern char *NV_strstr(const char *s, const char *sub);

// Copy two strings, ensuring NULL termination
static inline size_t NV_strcpy2(char *dest, const char *src) {
  return NV_strncpy2(dest, src, NV_strlen(dest));
}

// @return returns 0 when they are equal,
// positive number if the lc (last character) of s1 is greater than lc of s2
// negative number if the lc (last character) of s1 is less than lc of s2
extern int NV_strcmp(const char *s1, const char *s2);

// Return the number of characters after which 's' contains a character in 'reject'
// For example,
// s is Balling, reject is Hello
// so, strcspn will return 2 because after B and a,
// l is in both s and reject.
extern size_t NV_strcspn(const char *s, const char *reject);

// Return the number of characters after which s does NOT contain a character in accept
// ie. the number of similar characters they have.
// For example,
// s is Balling and accept is Ball
// strspn will return 4 because Ball is found in both s and accept and it is of 4 characters.
extern size_t NV_strspn(const char *s, const char *accept);

extern char *NV_strpbrk(const char *s1, const char *s2);

// Splits 's' by a delimiter
// warning: modifies s directly.
// so if you have obama-is-good-gamer,
// It'll first return to you 'obama', then 'is' them 'good' then 'gamer'
// You should pass NULL instead of the string for chaining calls
// like:
/*
  char buf[] = "obama-care-gaming";
  char *ptr = strtok(buf, '-'); // this will return 'obama'
  while (ptr != NULL) {
    ptr = strtok(NULL, '-'); // this will first return care, and in second iteration, return gaming
  }
*/
extern char *NV_strtok(char *s, const char *delim);

// Returns the NAME of the file
// basically, ../../pdf/nuclearlaunchcodes.pdf would give you nuclearlaunchcodes.pdf in return.
extern char *NV_basename(const char *path);

NOVA_HEADER_END;

#endif //__NOVA_STR_H__
