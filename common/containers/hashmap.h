#ifndef __NOVA_HASHMAP_H__
#define __NOVA_HASHMAP_H__

#include "../../common/mem.h"
#include <stdbool.h>
#include <stdio.h>

NOVA_HEADER_START;

typedef struct NV_hashmap_t NV_hashmap_t;
typedef unsigned (*NV_hashmap_hash_fn)(const void* bytes, int nbytes);
typedef bool (*NV_hashmap_key_equal_fn)(const void* key1, const void* key2, unsigned long keysize);

typedef struct ch_node_t
{
  void* key;
  void* value;
  bool  is_occupied;
} ch_node_t;

struct NV_hashmap_t
{
  unsigned                m_canary;
  ch_node_t**             m_nodes;
  NV_hashmap_hash_fn      m_hash_fn;
  NV_hashmap_key_equal_fn m_equal_fn;
  int                     m_entries, m_size;
  int                     m_key_size, m_value_size;
  pthread_rwlock_t        m_rwlock;
  NVAllocator*          allocator;
};

extern bool     NV_hashmap_std_key_eq(const void* key1, const void* key2, unsigned long nbytes);
extern unsigned NV_hashmap_std_hash(const void* bytes, int nbytes);

/*
    hash_fn may be NULL for the standard FNV-1A function.
    equal_fn may also be NULL for standard memcmp == 0
*/
extern NV_hashmap_t* NV_hashmap_init(int init_size, int keysize, int valuesize, NV_hashmap_hash_fn hash_fn, NV_hashmap_key_equal_fn equal_fn);

extern void          NV_hashmap_destroy(NV_hashmap_t* map);

extern void          NV_hashmap_resize(NV_hashmap_t* map, int new_size);

extern void          NV_hashmap_clear(NV_hashmap_t* map);

extern int           NV_hashmap_size(const NV_hashmap_t* map);

extern int           NV_hashmap_capacity(const NV_hashmap_t* map);

extern int           NV_hashmap_keysize(const NV_hashmap_t* map);

extern int           NV_hashmap_valuesize(const NV_hashmap_t* map);

// __i needs to point to an integer initialized to 0
extern ch_node_t*  NV_hashmap_iterate(const NV_hashmap_t* map, int* __i);

extern ch_node_t** NV_hashmap_root_node(const NV_hashmap_t* map);

/*
    WARNING: Doesn't replace the value if a key already exists!! Use NV_hashmap_insert_or_replace()
    also, if key or value is a string (const char *, not a NV_string_t or something),
    just pass in the const char *, not a pointer to it!!!
*/
extern void NV_hashmap_insert(NV_hashmap_t* map, const void* key, const void* value);

extern void NV_hashmap_insert_or_replace(NV_hashmap_t* map, const void* key, void* value);

/* returns NULL on no find */
extern void* NV_hashmap_find(const NV_hashmap_t* map, const void* key);

extern void  NV_hashmap_serialize(NV_hashmap_t* map, FILE* f);

extern void  NV_hashmap_deserialize(NV_hashmap_t* map, FILE* f);

NOVA_HEADER_END;

#endif //__NOVA_HASHMAP_H__