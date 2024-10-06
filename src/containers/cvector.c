#include <stdlib.h>

#include "../../include/containers/cvector.h"
#include "../../include/math/math.h"
#include "../../include/defines.h"

typedef struct cvector_t {
    int m_size;
    int m_capacity;
    int m_typesize;
    void *m_data;
} cvector_t;

cvector_t *cvector_init(int typesize, int init_size)
{
    cvector_t *vec = malloc(sizeof(struct cvector_t));
    cassert(vec != NULL);

    vec->m_size = 0;
    vec->m_typesize = typesize;
    vec->m_capacity = init_size;

    if (init_size > 0) {
        vec->m_data = calloc(init_size, typesize);
        cassert(vec->m_data != NULL);
    } else {
        vec->m_data = NULL;
    }

    return vec;
}

void cvector_destroy(cvector_t *vec)
{
    if (vec) {
        if (vec->m_data) {
            free(vec->m_data);
        }
        free(vec);
    }
}

void cvector_clear(cvector_t *vec)
{
    vec->m_size = 0;
}

int cvector_size(const cvector_t *vec)
{
    return vec->m_size;
}

int cvector_capacity(const cvector_t *vec)
{
    return vec->m_capacity;
}

int cvector_typesize(const cvector_t *vec)
{
    return vec->m_typesize;
}
void *cvector_data(const cvector_t *vec)
{
    return vec->m_data;
}

void *cvector_get(const cvector_t *vec, int i)
{
    return (uchar *)vec->m_data + (vec->m_typesize * i);
}

void cvector_set(cvector_t *vec, int i, void *elem)
{
    memcpy(cvector_get(vec, i), elem, vec->m_typesize);
}

void cvector_copy_from(const cvector_t *src, cvector_t *dst)
{
    cassert(src->m_typesize == dst->m_typesize);
    if (src->m_size >= dst->m_capacity) {
        cvector_resize(dst, src->m_size);
    }
    dst->m_size = src->m_size;
    memcpy(dst->m_data, src->m_data, src->m_size * src->m_typesize);
}

void cvector_move_from(cvector_t *src, cvector_t *dst)
{
    dst->m_size = src->m_size;
    dst->m_capacity = src->m_capacity;
    dst->m_data = src->m_data;

    src->m_size = 0;
    src->m_capacity = 0;
    src->m_data = NULL;
}

cvector_bool_t cvector_empty(const cvector_t *vec)
{
    return (vec->m_size == 0);
}

cvector_bool_t cvector_equal(const cvector_t *vec1, const cvector_t *vec2)
{
    if (vec1->m_size != vec2->m_size || vec1->m_typesize != vec2->m_typesize)
        return (memcmp(vec1->m_data, vec2->m_data, vec1->m_size * vec1->m_typesize) == 0);
    return 0;
}

void cvector_resize(cvector_t *vec, int new_size)
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

void cvector_push_back(cvector_t *vec, const void *elem)
{
    if ((vec->m_size + 1) >= vec->m_capacity) {
        cvector_resize(vec, cmmax(1, vec->m_capacity * 2));
    }
    memcpy((uchar *)vec->m_data + (vec->m_size * vec->m_typesize), elem, vec->m_typesize);
    vec->m_size++;
}

void cvector_push_set(cvector_t *vec, void *arr, int count)
{
    int required_capacity = vec->m_size + count;
    if (required_capacity >= vec->m_capacity)
        cvector_resize(vec, required_capacity);
    memcpy((uchar *)vec->m_data + (vec->m_size * vec->m_typesize), arr, count * vec->m_typesize);
    vec->m_size += count;
}

void cvector_pop_back(cvector_t *vec)
{
    if (vec->m_size > 0) {
        vec->m_size--;
    }
}

void cvector_pop_front(cvector_t *vec)
{
    if (vec->m_size > 0) {
        vec->m_size--;
        memcpy(vec->m_data, (uchar *)vec->m_data + vec->m_typesize, vec->m_size * vec->m_typesize);
    }
}

void cvector_insert(cvector_t *vec, int index, void *elem)
{
    if (index >= vec->m_capacity) {
        cvector_resize(vec, cmmax(1, index * 2)); // linear allocator. i could probably implement more but i don't have time rn
    }
    if (index >= vec->m_size) {
        vec->m_size = index + 1;
    }
    memcpy((uchar *)vec->m_data + (vec->m_typesize * index), elem, vec->m_typesize);
}

void cvector_remove(cvector_t *vec, int index)
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

int cvector_find(const cvector_t *vec, void *elem)
{
    for (int i = 0; i < vec->m_size; i++) {
        if (memcmp(vec->m_data + (i * vec->m_typesize), elem, vec->m_typesize)) {
            return i;
        }
    }
    return -1;
}
