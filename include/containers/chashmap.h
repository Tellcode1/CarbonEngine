#ifndef __HASHMAP_H
#define __HASHMAP_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct chashmap_t chashmap_t;
typedef unsigned (*chashmap_hash_fn)(const void *bytes, int nbytes);
typedef bool_t (*chashmap_key_equal_fn)(const void *key1, const void *key2, unsigned long keysize);

extern bool_t chashmap_std_key_eq(const void *key1, const void *key2, unsigned long nbytes);
extern unsigned chashmap_std_hash(const void *bytes, int nbytes);

/*
    hash_fn may be NULL for the standard FNV-1A function.
    equal_fn may also be NULL for standard memcmp == 0
*/
extern chashmap_t *chashmap_init(int init_size, int keysize, int valuesize, chashmap_hash_fn hash_fn, chashmap_key_equal_fn equal_fn);
extern void chashmap_destroy(chashmap_t *map);

extern void chashmap_resize(chashmap_t *map, int new_size);
extern void chashmap_clear(chashmap_t *map);

extern int chashmap_size(const chashmap_t *map);
extern int chashmap_capacity(const chashmap_t *map);
extern int chashmap_keysize(const chashmap_t *map);
extern int chashmap_valuesize(const chashmap_t *map);
extern void *chashmap_root_node(const chashmap_t *map);

/* WARNING: Doesn't replace the value if a key already exists!! Use chashmap_insert_or_replace() */
extern void chashmap_insert(chashmap_t *map, const void *key, void *value);

extern void chashmap_insert_or_replace(chashmap_t *map, const void *key, void *value);

/* returns NULL on no find */
extern void *chashmap_find(const chashmap_t *map, const void *key);

#ifdef __cplusplus
    }
#endif

#endif//__HASHMAP_H