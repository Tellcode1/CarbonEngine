#include <stdlib.h>

#include "../../include/cgvector.h"
#include "../../include/math/math.h"
#include "../../include/defines.h"

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

void *cg_vector_get(const cg_vector_t *vec, int i)
{
    return (uchar *)vec->m_data + (vec->m_typesize * i);
}

void cg_vector_set(cg_vector_t *vec, int i, void *elem)
{
    memcpy(cg_vector_get(vec, i), elem, vec->m_typesize);
}

void cg_vector_copy_from(const cg_vector_t *src, cg_vector_t *dst)
{
    cassert(src->m_typesize == dst->m_typesize);
    if (src->m_size >= dst->m_capacity) {
        cg_vector_resize(dst, src->m_size);
    }
    dst->m_size = src->m_size;
    memcpy(dst->m_data, src->m_data, src->m_size * src->m_typesize);
}

void cg_vector_move_from(cg_vector_t *src, cg_vector_t *dst)
{
    dst->m_size = src->m_size;
    dst->m_capacity = src->m_capacity;
    dst->m_data = src->m_data;

    src->m_size = 0;
    src->m_capacity = 0;
    src->m_data = NULL;
}

cg_vector_bool_t cg_vector_empty(const cg_vector_t *vec)
{
    return (vec->m_size == 0);
}

cg_vector_bool_t cg_vector_equal(const cg_vector_t *vec1, const cg_vector_t *vec2)
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

void cg_vector_push_back(cg_vector_t *vec, const void *elem)
{
    if ((vec->m_size + 1) >= vec->m_capacity) {
        cg_vector_resize(vec, cmmax(1, vec->m_capacity * 2));
    }
    memcpy((uchar *)vec->m_data + (vec->m_size * vec->m_typesize), elem, vec->m_typesize);
    vec->m_size++;
}

void cg_vector_push_set(cg_vector_t *vec, void *arr, int count)
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

void cg_vector_insert(cg_vector_t *vec, int index, void *elem)
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

int cg_vector_find(const cg_vector_t *vec, void *elem)
{
    for (int i = 0; i < vec->m_size; i++) {
        if (memcmp(vec->m_data + (i * vec->m_typesize), elem, vec->m_typesize)) {
            return i;
        }
    }
    return -1;
}
