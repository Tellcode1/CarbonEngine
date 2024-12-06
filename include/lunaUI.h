#ifndef __LUNA_UI_H
#define __LUNA_UI_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "lunaGFX.h"
#include "lunaPipeline.h"
#include "sprite.h"
#include "lunaDescriptors.h"
#include "lunaVK.h"
#include "cshadermanager.h"
#include "cgvector.h"
#include "world.h"
#include "lunaGPUObjects.h"
#include "camera.h"
#include "sprite.h"
#include "cinput.h"

typedef struct luna_UI_Button luna_UI_Button;

typedef void (*luna_UI_button_pressed_fn)(luna_UI_Button *bton);
typedef void (*luna_UI_button_hover_fn)(luna_UI_Button *bton);

typedef struct luna_UI_Context {
    luna_DescriptorSet set;
    cg_vector_t btons;
    void *ubmapped;
    luna_GPU_Sampler sampler;
    VkBuffer vb;
    VkBuffer ib;
    VkDeviceMemory vbm;
    VkDeviceMemory ibm;
} luna_UI_Context;

extern luna_UI_Context luna_ui_ctx;

typedef struct luna_UI_Button {
    vec2 position;
    vec2 size;
    vec4 color;
    luna_UI_button_hover_fn hover;
    luna_UI_button_pressed_fn pressed;
    sprite_t *spr;
    bool was_hovered; // was it being hovered in this frame?
    bool was_pressed; // was the button pressed?
} luna_UI_Button;

extern void luna_UI_Init();

extern luna_UI_Button *luna_UI_CreateButton(sprite_t *spr);

extern void luna_UI_Render(luna_Renderer_t *rd);

extern void luna_UI_Update();

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_UI_H