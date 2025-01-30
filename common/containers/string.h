#ifndef __NOVA_STRING_H__
#define __NOVA_STRING_H__

#include "../mem.h"

NOVA_HEADER_START;

typedef struct NV_string_t
{
  unsigned         m_canary;
  char*            m_data;
  size_t           m_size;
  size_t           m_capacity;
  pthread_rwlock_t m_rwlock;
  NVAllocator*   allocator;
} NV_string_t;

/*
    initial_size may be 0
*/
extern NV_string_t NV_string_init(size_t initial_size);
extern NV_string_t NV_string_init_str(const char* init);
extern NV_string_t NV_string_init_ptr(const char* begin, const char* end);
extern NV_string_t NV_string_substring(const NV_string_t* str, size_t start, size_t length);
extern void        NV_string_destroy(NV_string_t* str);

extern void        NV_string_clear(NV_string_t* str);
extern size_t      NV_string_length(const NV_string_t* str);
extern size_t      NV_string_capacity(const NV_string_t* str);
extern const char* NV_string_data(const NV_string_t* str);

extern void        NV_string_append(NV_string_t* str, const char* suffix);
extern void        NV_string_append_char(NV_string_t* str, char suffix);
extern void        NV_string_prepend(NV_string_t* str, const char* prefix);
extern void        NV_string_set(NV_string_t* str, const char* new_str);

/* returns -1 on no find */
extern size_t NV_string_find(const NV_string_t* str, const char* substr);
extern void   NV_string_remove(NV_string_t* str, size_t index, size_t length);

extern void   NV_string_copy_from(const NV_string_t* src, NV_string_t* dst);
// src is destroyed and unusable after this call!
extern void NV_string_move_from(NV_string_t* src, NV_string_t* dst);

NOVA_HEADER_END;

#endif // __NOVA_STRING_H__