#ifndef __LUNA_WORLD_H
#define __LUNA_WORLD_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "lunaGPUObjects.h"

typedef struct luna_World_t luna_World_t;

extern luna_World_t *luna_world_init();
extern void luna_world_render(luna_World_t *world);

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_WORLD_H