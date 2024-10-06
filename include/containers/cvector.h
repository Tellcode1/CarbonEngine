#ifndef __CVECTOR_H
#define __CVECTOR_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cvector_t cvector_t;
typedef unsigned char cvector_bool_t;

/*
    initial_size may be 0
*/
extern cvector_t *cvector_init(int typesize, int init_size);
extern void cvector_destroy(cvector_t *vec);

extern void cvector_resize(cvector_t *vec, int new_size);
extern void cvector_clear(cvector_t *vec);

extern int cvector_size(const cvector_t *vec);
extern int cvector_capacity(const cvector_t *vec);
extern int cvector_typesize(const cvector_t *vec);
extern void *cvector_data(const cvector_t *vec);

extern void *cvector_get(const cvector_t *vec, int i);
extern void cvector_set(cvector_t *vec, int i, void *elem);

// Overrides contents
extern void cvector_copy_from(const cvector_t *src, cvector_t *dst);
// src is destroyed and unusable after this call!
extern void cvector_move_from(cvector_t *src, cvector_t *dst);

extern cvector_bool_t cvector_empty(const cvector_t *vec);
extern cvector_bool_t cvector_equal(const cvector_t *vec1, const cvector_t *vec2);

// WARNING: sizeof(*elem) != vec->typesize is UNDEFINED!
extern void cvector_push_back(cvector_t *vec, const void *elem);
extern void cvector_push_set(cvector_t *vec, void *arr, int count);

extern void cvector_pop_back(cvector_t *vec);
extern void cvector_pop_front(cvector_t *vec); // expensive

extern void cvector_insert(cvector_t *vec, int index, void *elem);
extern void cvector_remove(cvector_t *vec, int index);

extern int cvector_find(const cvector_t *vec, void *elem);

#ifdef __cplusplus
    }
#endif

#endif//__CVECTOR_H