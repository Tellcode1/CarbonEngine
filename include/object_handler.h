#ifndef __OBJECT_HANDLER_H
#define __OBJECT_HANDLER_H

#include "cimage.h"
#include "cgvector.h"
#include "math/vec2.h"

typedef enum primitive_type {
    QUAD = 0,
    CIRCLE = 1,
    IDK
} primitive_type;

typedef struct obj_transform {
    vec2 position;
    vec2 scale;
    float rotation;
} obj_transform;

typedef struct object {
    const char *name;
    primitive_type type;
    cg_tex2D *texture; // pointer so can be used by multiple
} object;

typedef struct obj_handler {
    cg_vector_t objects; // of struct object
} obj_handler;

static obj_handler make_handler() {
    obj_handler ret = {};
    ret.objects = cg_vector_init(sizeof(object), 16);
}

#endif//__OBJECT_HANDLER_H