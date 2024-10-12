#include <stdlib.h>

#include "../../include/cghashmap.h"
#include "../../include/defines.h"

typedef struct ch_node_t {
    void *key;
    void *value;
    unsigned char is_occupied;
} ch_node_t;

typedef struct cg_hashmap_t {
    ch_node_t **nodes;
    cg_hashmap_hash_fn hash_fn;
    cg_hashmap_key_equal_fn equal_fn;
    int entries, size;
    int keysize, valuesize;
} cg_hashmap_t;

unsigned closest_power_of_two(unsigned i) {
    if (i == 0) {
        return 1;
    }
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i++;
    return i;
}

unsigned int power_of_two_mod(unsigned int x, unsigned int n) {
    return x & (n - 1);
}

unsigned cg_hashmap_std_hash(const void *bytes, int nbytes) {
    const unsigned FNV_PRIME = 16777619;
    const unsigned OFFSET_BASIS = 2166136261;

    unsigned hash = OFFSET_BASIS;
    for (unsigned char byte = 0; byte < nbytes; byte++) {
        hash ^= ((unsigned char *)bytes)[byte]; // xor
        hash *= FNV_PRIME;
    }
    return hash;
};

bool_t cg_hashmap_std_key_eq(const void *key1, const void *key2, unsigned long nbytes) {
    if (key1 == key2) {
        return 1;
    } else {
        return memcmp(key1, key2, nbytes) == 0;
    }
}

cg_hashmap_t *cg_hashmap_init(int init_size, int keysize, int valuesize, cg_hashmap_hash_fn hash_fn, cg_hashmap_key_equal_fn equal_fn)
{
    cassert(keysize > 0 && valuesize > 0);

    cg_hashmap_t *map = malloc(sizeof(struct cg_hashmap_t));
    cassert(map != NULL);
    memset(map, 0, sizeof(struct cg_hashmap_t));

    if (init_size < 0) {
        init_size = 1;
    }
    if (hash_fn == NULL) {
        hash_fn = cg_hashmap_std_hash;
    }
    if (equal_fn == NULL) {
        equal_fn = cg_hashmap_std_key_eq;
    }

    map->nodes = (ch_node_t **)calloc(init_size, sizeof(ch_node_t));
    cassert(map->nodes != NULL);

    map->hash_fn = hash_fn;
    map->equal_fn = equal_fn;
    map->keysize = keysize;
    map->valuesize = valuesize;
    map->entries = closest_power_of_two(init_size);
    map->size = 0;

    return map;
}

void cg_hashmap_destroy(cg_hashmap_t *map)
{
    if (!map->nodes) {
        return;
    }
    for (int i = 0; i < map->entries; i++) {
        if (map->nodes[i]) {
            free(map->nodes[i]);
        }
    }
    free(map->nodes);
}

void cg_hashmap_resize(cg_hashmap_t *map, int new_size)
{
    ch_node_t **old_nodes = map->nodes;
    int old_entries = map->entries;

    if (new_size <= 0) {
        new_size = 1;
    }

    map->entries = closest_power_of_two(new_size);
    map->size = 0;

    map->nodes = calloc(new_size, sizeof(ch_node_t));
    cassert(map->nodes != NULL);

    if (old_nodes) {
        for (int i = 0; i < old_entries; i++) {
            ch_node_t *node = old_nodes[i];
            if (node && node->is_occupied) {
                cg_hashmap_insert(map, node->key, node->value);
                free(node);
            }
        }
        free(old_nodes);
    }
}


void cg_hashmap_clear(cg_hashmap_t *map)
{
    if (!map->nodes) {
        return;
    }
    for (int i = 0; i < map->entries; i++) {
        if (map->nodes[i]) {
            free(map->nodes[i]);
        }
    }
    free(map->nodes);
    map->nodes = NULL;
    map->size = 0;
    map->entries = 0;
}

int cg_hashmap_size(const cg_hashmap_t *map)
{
    return map->size;
}

int cg_hashmap_capacity(const cg_hashmap_t *map)
{
    return map->entries;
}

int cg_hashmap_keysize(const cg_hashmap_t *map)
{
    return map->keysize;
}

int cg_hashmap_valuesize(const cg_hashmap_t *map)
{
    return map->valuesize;
}

void *cg_hashmap_root_node(const cg_hashmap_t *map)
{
    return map->nodes;
}

void *cg_hashmap_find(const cg_hashmap_t *map, const void *key)
{
    if (!map->nodes) {
        return NULL;
    }

    const unsigned begin = (map->hash_fn(key, map->keysize) % map->entries);
    unsigned i = begin;
    while (map->nodes[i] != NULL && map->nodes[i]->is_occupied) {
        if (map->equal_fn(map->nodes[i]->key, key, map->keysize)) {
            return map->nodes[i]->value;
        }
        i = power_of_two_mod((i + 1), map->entries);
        if (i == begin) {
            break;
        }
    }
    return NULL;
}

void cg_hashmap_insert(cg_hashmap_t *map, const void *key, void *value)
{
    if (!map->nodes || map->size >= (map->entries * 3) / 4) {
        // The check to whether map->entries is greater than 0 is already done in resize();
        cg_hashmap_resize(map, map->entries * 2);
    }

    const unsigned begin = power_of_two_mod(map->hash_fn(key, map->keysize), map->entries);
    unsigned i = begin;
    while (map->nodes[i] && map->nodes[i]->is_occupied) {
        i = power_of_two_mod((i + 1), map->entries);
        if (i == begin) {
            break;
        }
    }

    if (!map->nodes[i]) {
        // Batch allocation for the entire node at once.
        void *alloc = malloc(sizeof(ch_node_t) + map->keysize + map->valuesize);
        map->nodes[i] = alloc;
        map->nodes[i]->key = alloc + sizeof(ch_node_t);
        map->nodes[i]->value = alloc + sizeof(ch_node_t) + map->keysize;
    }

    memcpy(map->nodes[i]->key, key, map->keysize);
    memcpy(map->nodes[i]->value, value, map->valuesize);
    map->nodes[i]->is_occupied = 1;
    map->size++;
}

void cg_hashmap_insert_or_replace(cg_hashmap_t *map, const void *key, void *value)
{
    if (!map->nodes || map->size >= (map->entries * 3) / 4) {
        // The check to whether map->entries is greater than 0 is already done in resize();
        cg_hashmap_resize(map, map->entries * 2);
    }

    const unsigned begin = power_of_two_mod(map->hash_fn(key, map->keysize), map->entries);
    unsigned i = begin;
    while (map->nodes[i] && map->nodes[i]->is_occupied) {
        i = power_of_two_mod((i + 1), map->entries);
        if (map->equal_fn(map->nodes[i]->key, key, map->keysize)) {
            memcpy(map->nodes[i]->value, value, map->valuesize);
        }
        else if (i == begin) {
            break;
        }
    }

    if (!map->nodes[i]) {
        // Batch allocation for the entire node at once.
        void *alloc = malloc(sizeof(ch_node_t) + map->keysize + map->valuesize);
        map->nodes[i] = alloc;
        map->nodes[i]->key = alloc + sizeof(ch_node_t);
        map->nodes[i]->value = alloc + sizeof(ch_node_t) + map->keysize;
    }

    memcpy(map->nodes[i]->key, key, map->keysize);
    memcpy(map->nodes[i]->value, value, map->valuesize);
    map->nodes[i]->is_occupied = 1;
    map->size++;
}