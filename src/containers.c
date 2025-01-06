#include "../include/containers/cgvector.h"
#include "../include/containers/cgstring.h"
#include "../include/containers/cghashmap.h"
#include "../include/containers/cgbitset.h"
#include "../include/containers/catlas.h"

#include <stdlib.h>

#include "../../include/containers/cgvector.h"
#include "../../include/math/math.h"
#include "../../include/stdafx.h"

#ifndef cg_cont_alloc
    #define cg_cont_alloc malloc
#endif

#ifndef cg_cont_calloc
    #define cg_cont_calloc calloc
#endif

#ifndef cg_cont_realloc
    #define cg_cont_realloc realloc
#endif

#ifndef cg_cont_free
    #define cg_cont_free free
#endif

// ==============================
// VECTOR
// ==============================

cg_vector_t cg_vector_init(int typesize, int init_size)
{
    cg_vector_t vec = {};
    vec.m_size = 0;
    vec.m_typesize = typesize;
    vec.m_capacity = init_size;

    if (init_size > 0) {
        vec.m_data = calloc(init_size, typesize);
        cassert(vec.m_data != NULL);
    } else {
        vec.m_data = NULL;
    }

    return vec;
}

void cg_vector_destroy(cg_vector_t *vec)
{
    if (vec) {
        if (vec->m_data) {
            free(vec->m_data);
        }
    }
}

void cg_vector_clear(cg_vector_t *vec)
{
    // we don't remove all data here to reduce allocations but this can POSSIBLY hold tons of garbage data
    vec->m_size = 0;
}

int cg_vector_size(const cg_vector_t *vec)
{
    return vec->m_size;
}

int cg_vector_capacity(const cg_vector_t *vec)
{
    return vec->m_capacity;
}

int cg_vector_typesize(const cg_vector_t *vec)
{
    return vec->m_typesize;
}
void *cg_vector_data(const cg_vector_t *vec)
{
    return vec->m_data;
}

void *cg_vector_back(cg_vector_t *vec)
{
    return cg_vector_get(vec, cmmax(0, cg_vector_size(vec) - 1));
}

void *cg_vector_get(const cg_vector_t *vec, int i)
{
    return (uchar *)vec->m_data + (vec->m_typesize * i);
}

void cg_vector_set(cg_vector_t *vec, int i, void *elem)
{
    memcpy(cg_vector_get(vec, i), elem, vec->m_typesize);
}

void cg_vector_copy_from(const cg_vector_t *__restrict src, cg_vector_t *__restrict dst)
{
    cassert(src->m_typesize == dst->m_typesize);
    if (src->m_size >= dst->m_capacity) {
        cg_vector_resize(dst, src->m_size);
    }
    dst->m_size = src->m_size;
    memcpy(dst->m_data, src->m_data, src->m_size * src->m_typesize);
}

void cg_vector_move_from(cg_vector_t *__restrict src, cg_vector_t *__restrict dst)
{
    dst->m_size = src->m_size;
    dst->m_capacity = src->m_capacity;
    dst->m_data = src->m_data;

    src->m_size = 0;
    src->m_capacity = 0;
    src->m_data = NULL;
}

cg_vector_bool cg_vector_empty(const cg_vector_t *vec)
{
    return (vec->m_size == 0);
}

cg_vector_bool cg_vector_equal(const cg_vector_t *vec1, const cg_vector_t *vec2)
{
    if (vec1->m_size != vec2->m_size || vec1->m_typesize != vec2->m_typesize)
        return (memcmp(vec1->m_data, vec2->m_data, vec1->m_size * vec1->m_typesize) == 0);
    return 0;
}

void cg_vector_resize(cg_vector_t *vec, int new_size)
{
    // realloc crashes if m_data is NULL.
    if (vec->m_data) {
        vec->m_data = realloc(vec->m_data, vec->m_typesize * new_size);
    } else {
        vec->m_data = calloc(vec->m_typesize, new_size);
        /* vec->m_data is NULL so we don't really have anything to copy */
    }
    cassert(vec->m_data != NULL);

    vec->m_capacity = new_size;
}

void cg_vector_push_back(cg_vector_t *__restrict vec, const void *__restrict elem)
{
    if ((vec->m_size + 1) >= vec->m_capacity) {
        cg_vector_resize(vec, cmmax(1, vec->m_capacity * 2));
    }
    memcpy((uchar *)vec->m_data + (vec->m_size * vec->m_typesize), elem, vec->m_typesize);
    vec->m_size++;
}

void cg_vector_push_set(cg_vector_t *__restrict vec, const void *__restrict arr, int count)
{
    int required_capacity = vec->m_size + count;
    if (required_capacity >= vec->m_capacity)
        cg_vector_resize(vec, required_capacity);
    memcpy((uchar *)vec->m_data + (vec->m_size * vec->m_typesize), arr, count * vec->m_typesize);
    vec->m_size += count;
}

void cg_vector_pop_back(cg_vector_t *vec)
{
    if (vec->m_size > 0) {
        vec->m_size--;
    }
}

void cg_vector_pop_front(cg_vector_t *vec)
{
    if (vec->m_size > 0) {
        vec->m_size--;
        memcpy(vec->m_data, (uchar *)vec->m_data + vec->m_typesize, vec->m_size * vec->m_typesize);
    }
}

void cg_vector_insert(cg_vector_t *__restrict vec, int index, const void *__restrict elem)
{
    if (index >= vec->m_capacity) {
        cg_vector_resize(vec, cmmax(1, index * 2)); // linear allocator. i could probably implement more but i don't have time rn
    }
    if (index >= vec->m_size) {
        vec->m_size = index + 1;
    }
    memcpy((uchar *)vec->m_data + (vec->m_typesize * index), elem, vec->m_typesize);
}

void cg_vector_remove(cg_vector_t *vec, int index)
{
    if (index >= vec->m_size)
        return;
    else if (vec->m_size - index - 1) {
        // please don't ask me what this is
        memcpy(
            (uchar *)vec->m_data + (index * vec->m_typesize),
            (uchar *)vec->m_data + ((index + 1) * vec->m_typesize),
            (vec->m_size - index - 1) * vec->m_typesize
        );
    }
    vec->m_size--;
}

int cg_vector_find(const cg_vector_t *__restrict vec, const void *__restrict elem)
{
    for (int i = 0; i < vec->m_size; i++) {
        if (memcmp(vec->m_data + (i * vec->m_typesize), elem, vec->m_typesize)) {
            return i;
        }
    }
    return -1;
}

void cg_vector_sort(cg_vector_t *vec, cg_vector_compare_fn compare)
{
    qsort(vec->m_data, vec->m_size, vec->m_typesize, compare);
}

// ==============================
// STRING
// ==============================

static void cg_string_resize(cg_string_t *str, int new_capacity) {
    char *new_data = cg_cont_realloc(str->data, new_capacity);
    cassert(new_data != NULL);
    str->data = new_data;
    str->capacity = new_capacity;
}

cg_string_t *cg_string_init(int initial_size) {
    cg_string_t *str = cg_cont_alloc(sizeof(cg_string_t));
    cassert(str != NULL);
    
    str->capacity = (initial_size > 0) ? initial_size : 1;
    str->data = cg_cont_alloc(str->capacity);
    cassert(str->data != NULL);
    
    str->data[0] = '\0';
    str->length = 0;
    return str;
}

cg_string_t *cg_string_init_str(const char *init) {
    cassert(init != NULL && strlen(init) > 0);
    cg_string_t *str = cg_cont_alloc(sizeof(cg_string_t));
    cassert(str != NULL);
    
    int len = strlen(init);
    str->capacity = len + 1;
    str->data = cg_cont_alloc(str->capacity);
    cassert(str->data != NULL);
    
    strcpy(str->data, init);
    str->length = len;
    
    return str;
}

cg_string_t *cg_string_substring(const cg_string_t *str, int start, int length) {
    cassert(str != NULL);
    cassert(start >= 0 && start + length <= str->length);

    cg_string_t *substr = cg_string_init(length + 1);
    cassert(substr != NULL);

    strncpy(substr->data, str->data + start, length);
    substr->data[length] = '\0';
    substr->length = length;
    return substr;
}

void cg_string_destroy(cg_string_t *str) {
    if (str) {
        cg_cont_free(str->data);
        cg_cont_free(str);
    }
}

void cg_string_clear(cg_string_t *str) {
    if (str) {
        str->length = 0;
        str->data[0] = '\0';
    }
}

int cg_string_length(const cg_string_t *str) {
    cassert(str != NULL);
    return str->length;
}

int cg_string_capacity(const cg_string_t *str) {
    cassert(str != NULL);
    return str->capacity;
}

const char *cg_string_data(const cg_string_t *str) {
    cassert(str != NULL);
    return str->data;
}

void cg_string_append(cg_string_t *str, const char *suffix) {
    cassert(str != NULL);
    cassert(suffix != NULL);
    
    int suffix_length = strlen(suffix);
    if (str->length + suffix_length + 1 > str->capacity) {
        cg_string_resize(str, str->length + suffix_length + 1);
    }

    strcpy(str->data + str->length, suffix);
    str->length += suffix_length;
}

void cg_string_append_char(cg_string_t *str, char suffix) {
    cassert(str != NULL);
    
    if (str->length + 2 > str->capacity) {
        cg_string_resize(str, str->length + 2);
    }

    str->data[str->length] = suffix;
    str->length++;
    str->data[str->length] = '\0';
}

void cg_string_prepend(cg_string_t *str, const char *prefix) {
    cassert(str != NULL);
    cassert(prefix != NULL);

    int prefix_length = strlen(prefix);
    if (str->length + prefix_length + 1 > str->capacity) {
        cg_string_resize(str, str->length + prefix_length + 1);
    }

    memmove(str->data + prefix_length, str->data, str->length + 1);
    memcpy(str->data, prefix, prefix_length);
    str->length += prefix_length;
}

void cg_string_set(cg_string_t *str, const char *new_str) {
    cassert(str != NULL);
    cassert(new_str != NULL);

    int new_length = strlen(new_str);
    if (new_length + 1 > str->capacity) {
        cg_string_resize(str, new_length + 1);
    }

    strcpy(str->data, new_str);
    str->length = new_length;
}

int cg_string_find(const cg_string_t *str, const char *substr) {
    cassert(str != NULL);
    cassert(substr != NULL);

    char *pos = strstr(str->data, substr);
    return pos ? (int)(pos - str->data) : -1;
}

void cg_string_remove(cg_string_t *str, int index, int length) {
    cassert(str != NULL);
    cassert(index >= 0 && index < str->length);

    if (index + length > str->length) {
        length = str->length - index;
    }

    memmove(str->data + index, str->data + index + length, str->length - index - length + 1);
    str->length -= length;
}

void cg_string_copy_from(const cg_string_t *src, cg_string_t *dst) {

    cassert(src != NULL);
    cassert(dst != NULL);
    cg_string_set(dst, src->data);
}

void cg_string_move_from(cg_string_t *src, cg_string_t *dst) {
    cassert(src != NULL);
    cassert(dst != NULL);
    cg_string_copy_from(src, dst);
    cg_string_destroy(src);
}

// ==============================
// HASHMAP
// ==============================

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

bool cg_hashmap_std_key_eq(const void *key1, const void *key2, unsigned long nbytes) {
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
    (*map) = (cg_hashmap_t){};

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
    const int old_entries = map->entries;

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

ch_node_t **cg_hashmap_root_node(const cg_hashmap_t *map)
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

void cg_hashmap_insert(cg_hashmap_t *map, const void *key, const void *value)
{
    // the second check 
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

void cg_hashmap_serialize(cg_hashmap_t *map, FILE *f) {
    const int key_size = map->keysize;
    const int val_size = map->valuesize;

    for (int i = 0; i < map->entries; i++) {
        if (map->nodes[i] && map->nodes[i]->is_occupied) {
            void *node_key = map->nodes[i]->key;
            void *node_value = map->nodes[i]->value;

            fwrite(node_value, val_size, 1, f);
            fwrite(node_key, key_size, 1, f);
        }
    }
}

void cg_hashmap_read(cg_hashmap_t *map, FILE *f) {
    void *key = malloc(map->keysize);
    void *value = malloc(map->valuesize);

    while (fread(value, map->valuesize, 1, f) == 1 && fread(key, map->keysize, 1, f) == 1) {
        cg_hashmap_insert(map, key, value);
    }

    free(key);
    free(value);
}

// ==============================
// ATLAS
// ==============================

catlas_t catlas_init(int init_w, int init_h)
{
    catlas_t atlas = {};

    atlas.width = init_w;
    atlas.height = init_h;
    atlas.next_x = 0;
    atlas.next_y = 0;
    atlas.current_row_height = 0;
    atlas.data = calloc(init_w, init_h);

    return atlas;
}

bool catlas_add_image(catlas_t *__restrict__ atlas, int w, int h, const unsigned char *__restrict__ data, int *__restrict__ x, int *__restrict__ y)
{
    const int padding = 4;
    const int prev_h = atlas->height, prev_w = atlas->width;
    bool realloc_needed = 0;
    if (w > atlas->width) {
        // ! This doesn't work because the old image is not correctly copied by realloc
        // ! It'll be fixed by copying over the data row by row
        // probably doesn't need fixing right now, will delay it for another eon
        // TODO: FIXME
        atlas->width = w;
        realloc_needed = 1;
    }

    if (atlas->next_x + w + padding > atlas->width) {
        atlas->next_x = 0;
        atlas->next_y += atlas->current_row_height + padding;
        atlas->current_row_height = 0;
    }

    if (atlas->next_y + h + padding > atlas->height) {
        atlas->height = cmmax(atlas->height * 2, atlas->next_y + h + padding);
        realloc_needed = 1;
    }

    if (realloc_needed) {
        atlas->data = realloc(atlas->data, atlas->width * atlas->height);
        memset(atlas->data + prev_w * prev_h, 0, (atlas->width - prev_w) * (atlas->height - prev_h));
    }

    for (int y = 0; y < h; y++) {
        memcpy(atlas->data + (atlas->next_x + (atlas->next_y + y) * atlas->width), data + (y * w), w);
    }

    *x = atlas->next_x;
    *y = atlas->next_y;

    atlas->next_x += w + padding;
    atlas->current_row_height = cmmax(atlas->current_row_height, h + padding);

    return 0;
}

// ==============================
// BITSET
// ==============================

cg_bitset_t cg_bitset_init(int init_capacity)
{   
    cg_bitset_t ret = {};
    if (init_capacity > 0) {
        init_capacity = (init_capacity + 7) / 8;
        ret.size = init_capacity;
        ret.data = calloc(init_capacity, sizeof(uint8_t));
    } else {
        ret.size = 0;
    }
    return ret;
}

void cg_bitset_set_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] |= (1 << (bitindex % 8));
}

void cg_bitset_set_bit_to(cg_bitset_t *set, int bitindex, cg_bitset_bit to)
{
    to ?
        cg_bitset_set_bit(set, bitindex)
        :
        cg_bitset_clear_bit(set, bitindex);
}

void cg_bitset_clear_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] &= ~(1 << (bitindex % 8));
}

void cg_bitset_toggle_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] ^= (1 << (bitindex % 8));
}

cg_bitset_bit cg_bitset_access_bit(cg_bitset_t *set, int bitindex)
{
    return (set->data[bitindex / 8] & (1 << (bitindex % 8))) != 0;
}

void cg_bitset_copy_from(cg_bitset_t *dst, const cg_bitset_t *src)
{
    if (src->size != dst->size && dst->data) {
        free(dst->data);
        dst->data = malloc(src->size);
        dst->size = src->size;
    }
    memcpy(dst->data, src->data, src->size);
}

void cg_bitset_destroy(cg_bitset_t *set)
{
    free(set->data);
    free(set);
}
