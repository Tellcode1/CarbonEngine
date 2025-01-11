#ifndef __LUNA_STR_H__
#define __LUNA_STR_H__

#include <stdint.h>

// This is literally using the 'n' types of the string functions

extern size_t luna_strlen(const char *s);

extern size_t luna_strncpy(char *dest, const char *src, size_t max);
extern char *luna_strnchr(char *str, int chr, size_t max);
extern char *luna_strrchr(char *str, int chr);
extern int luna_strncmp(const char *s1, const char *s2, size_t max);

static inline size_t luna_strcpy(char *dest, const char *src) {
  return luna_strncpy(dest, src, SIZE_MAX);
}
static inline char *luna_strchr(char *str, int chr) {
  return luna_strnchr(str, chr, strlen(str));
}
static inline int luna_strcmp(const char *s1, const char *s2) {
  return luna_strncmp(s1, s2, strlen(s1));
}

#endif //__LUNA_STR_H__