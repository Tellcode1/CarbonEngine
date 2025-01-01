#ifndef __LUNA_UI_H
#define __LUNA_UI_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "math/vec2.h"
#include "math/vec4.h"

#include "sprite.h"
#include "lunaDescriptors.h"
#include "lunaGPUObjects.h"

typedef struct luna_Renderer_t luna_Renderer_t;
typedef struct luna_UI_Button luna_UI_Button;

typedef void (*luna_UI_button_pressed_fn)(luna_UI_Button *bton);
typedef void (*luna_UI_button_hover_fn)(luna_UI_Button *bton);

extern struct luna_UI_Context luna_ui_ctx;

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

typedef struct luna_UI_Slider {
    vec2 position, size;
    vec4 bg_color, slider_color;
    sprite_t *bg_sprite, *slider_sprite;
    float min,max,value;
    bool moved; // was the slider's handle moved
    bool interactable; // whether this slider can be controlled by the mouse. default off
} luna_UI_Slider;

extern void luna_UI_Init();

extern luna_UI_Button *luna_UI_CreateButton(struct sprite_t *spr);

extern luna_UI_Slider *luna_UI_CreateSlider();

extern void luna_UI_Render(luna_Renderer_t *rd);

extern void luna_UI_Update();

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_UI_H