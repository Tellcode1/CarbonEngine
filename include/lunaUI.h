#ifndef __LUNA_UI_H
#define __LUNA_UI_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "math/vec2.h"
#include "lunaPipeline.h"
#include "undescriptorset.h"
#include "cgvector.h"

typedef void (*luna_UI_button_pressed_fn)(struct luna_UI_Button *bton);
typedef void (*luna_UI_button_hover_fn)(struct luna_UI_Button *bton);

typedef struct luna_UI_Context {
    luna_VK_Pipeline pipeline;
    undescriptorset set;
    cg_vector_t btons;
} luna_UI_Context;

static luna_UI_Context luna_ui_ctx;

typedef struct luna_UI_Button {
    vec2 position;
    vec2 size;
    luna_UI_button_hover_fn hover;
    luna_UI_button_pressed_fn pressed;
} luna_UI_Button;

void luna_UI_Init() {
    
}

void luna_UI_CreateButton(luna_UI_Button *bton) {
    *bton = (luna_UI_Button){};
    cg_vector_push_back(&luna_ui_ctx.btons, bton);
}

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_UI_H