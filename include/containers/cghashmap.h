#ifndef __CG_HASHMAP_H
#define __CG_HASHMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>

typedef struct ch_node_t {
  void *key;
  void *value;
  bool is_occupied;
} ch_node_t;

typedef struct cg_hashmap_t cg_hashmap_t;
typedef unsigned (*cg_hashmap_hash_fn)(const void *bytes, int nbytes);
typedef bool (*cg_hashmap_key_equal_fn)(const void *key1, const void *key2, unsigned long keysize);

extern bool cg_hashmap_std_key_eq(const void *key1, const void *key2, unsigned long nbytes);
extern unsigned cg_hashmap_std_hash(const void *bytes, int nbytes);

/*
    hash_fn may be NULL for the standard FNV-1A function.
    equal_fn may also be NULL for standard memcmp == 0
*/
extern cg_hashmap_t *cg_hashmap_init(int init_size, int keysize, int valuesize, cg_hashmap_hash_fn hash_fn, cg_hashmap_key_equal_fn equal_fn);
extern void cg_hashmap_destroy(cg_hashmap_t *map);

extern void cg_hashmap_resize(cg_hashmap_t *map, int new_size);
extern void cg_hashmap_clear(cg_hashmap_t *map);

extern int cg_hashmap_size(const cg_hashmap_t *map);
extern int cg_hashmap_capacity(const cg_hashmap_t *map);
extern int cg_hashmap_keysize(const cg_hashmap_t *map);
extern int cg_hashmap_valuesize(const cg_hashmap_t *map);
extern ch_node_t *cg_hashmap_iterate(const cg_hashmap_t *map, int *__i);
extern ch_node_t **cg_hashmap_root_node(const cg_hashmap_t *map);

/*
    WARNING: Doesn't replace the value if a key already exists!! Use cg_hashmap_insert_or_replace()
    also, if key or value is a string (const char *, not a cg_string_t or something),
    just pass in the const char *, not a pointer to it!!!
*/
extern void cg_hashmap_insert(cg_hashmap_t *map, const void *key, const void *value);

extern void cg_hashmap_insert_or_replace(cg_hashmap_t *map, const void *key, void *value);

/* returns NULL on no find */
extern void *cg_hashmap_find(const cg_hashmap_t *map, const void *key);

extern void cg_hashmap_serialize(cg_hashmap_t *map, FILE *f);
extern void cg_hashmap_read(cg_hashmap_t *map, FILE *f);

#ifdef __cplusplus
}
#endif

#endif //__CG_HASHMAP_H