#ifndef __CG_VECTOR_H
#define __CG_VECTOR_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cg_vector_t {
    int m_size;
    int m_capacity;
    int m_typesize;
    void *m_data;
} cg_vector_t;
typedef int (*cg_vector_compare_fn)(const void *obj1, const void *obj2);

typedef unsigned char cg_vector_bool;

/*
    initial_size may be 0
*/
extern cg_vector_t cg_vector_init(int typesize, int init_size);
extern void cg_vector_destroy(cg_vector_t *vec);

extern void cg_vector_resize(cg_vector_t *vec, int new_size);
extern void cg_vector_clear(cg_vector_t *vec);

extern int cg_vector_size(const cg_vector_t *vec);
extern int cg_vector_capacity(const cg_vector_t *vec);
extern int cg_vector_typesize(const cg_vector_t *vec);
extern void *cg_vector_data(const cg_vector_t *vec);

extern void *cg_vector_back(cg_vector_t *vec);

extern void *cg_vector_get(const cg_vector_t *vec, int i);
extern void cg_vector_set(cg_vector_t *vec, int i, void *elem);

// Overrides contents
extern void cg_vector_copy_from(const cg_vector_t *__restrict src, cg_vector_t *__restrict dst);
// src is destroyed and unusable after this call!
extern void cg_vector_move_from(cg_vector_t *__restrict src, cg_vector_t *__restrict dst);

extern cg_vector_bool cg_vector_empty(const cg_vector_t *vec);

// Doesn't mean that the two vectors ptrs are pointing to the same vector
// This'll check the size and the data only, not the capacity (why would you?)
extern cg_vector_bool cg_vector_equal(const cg_vector_t *vec1, const cg_vector_t *vec2);

// WARNING: sizeof(*elem) != vec->typesize is UNDEFINED!
extern void cg_vector_push_back(cg_vector_t *__restrict vec, const void *__restrict elem);
extern void cg_vector_push_set(cg_vector_t *__restrict vec, const void *__restrict arr, int count);

extern void cg_vector_pop_back(cg_vector_t *vec);
extern void cg_vector_pop_front(cg_vector_t *vec); // expensive

extern void cg_vector_insert(cg_vector_t *__restrict vec, int index, const void *__restrict elem);
extern void cg_vector_remove(cg_vector_t *vec, int index);

extern int cg_vector_find(const cg_vector_t *__restrict vec, const void *__restrict elem);

extern void cg_vector_sort(cg_vector_t *vec, cg_vector_compare_fn compare);

#ifdef __cplusplus
    }
#endif

#endif//__CG_VECTOR_H