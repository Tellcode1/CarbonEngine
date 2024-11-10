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

extern void *cg_vector_get(const cg_vector_t *vec, int i);
extern void cg_vector_set(cg_vector_t *vec, int i, void *elem);

// Overrides contents
extern void cg_vector_copy_from(const cg_vector_t *src, cg_vector_t *dst);
// src is destroyed and unusable after this call!
extern void cg_vector_move_from(cg_vector_t *src, cg_vector_t *dst);

extern cg_vector_bool cg_vector_empty(const cg_vector_t *vec);
extern cg_vector_bool cg_vector_equal(const cg_vector_t *vec1, const cg_vector_t *vec2);

// WARNING: sizeof(*elem) != vec->typesize is UNDEFINED!
extern void cg_vector_push_back(cg_vector_t *vec, const void *elem);
extern void cg_vector_push_set(cg_vector_t *vec, void *arr, int count);

extern void cg_vector_pop_back(cg_vector_t *vec);
extern void cg_vector_pop_front(cg_vector_t *vec); // expensive

extern void cg_vector_insert(cg_vector_t *vec, int index, void *elem);
extern void cg_vector_remove(cg_vector_t *vec, int index);

extern int cg_vector_find(const cg_vector_t *vec, void *elem);

#ifdef __cplusplus
    }
#endif

#endif//__CG_VECTOR_H