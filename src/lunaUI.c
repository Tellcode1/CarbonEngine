#include "../include/lunaUI.h"
#include "../include/lunaGFX.h"
#include "../include/lunaDescriptors.h"
#include "../include/camera.h"
#include "../include/cinput.h"

typedef struct luna_UI_Context {
    cg_vector_t btons;
    void *ubmapped;
} luna_UI_Context;

luna_UI_Context luna_ui_ctx;

void luna_UI_Init() {
    luna_ui_ctx.btons = cg_vector_init(sizeof(luna_UI_Button), 4);
}

luna_UI_Button *luna_UI_CreateButton(sprite_t *spr) {
    luna_UI_Button bton = (luna_UI_Button){};
    bton.position = (vec2){};
    bton.size = (vec2){1.0f,1.0f};
    bton.color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
    bton.spr = spr;
    cg_vector_push_back(&luna_ui_ctx.btons, &bton);
    return &((luna_UI_Button *)cg_vector_data(&luna_ui_ctx.btons))[ cg_vector_size(&luna_ui_ctx.btons) - 1 ];
}

void luna_UI_Render(luna_Renderer_t *rd) {
    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++) {
        const luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);

        luna_Renderer_DrawQuad(
            rd,
            bton->spr,
            (vec2){ 1.0f, 1.0f },
            (vec3){ bton->position.x, bton->position.y, 0.0f },
            (vec3){ bton->size.x, bton->size.y, 1.0f },
            bton->color,
            0
        );
    }
}

void luna_UI_Update() {
    const bool mouse_pressed = cinput_is_mouse_pressed(SDL_BUTTON_LEFT);
    const cmrect2d mouse = {
        .position = v2add(
            v2mulv(cinput_get_mouse_position(), (vec2){ CAMERA_ORTHO_W, CAMERA_ORTHO_H }),
            (vec2){ camera.position.x, camera.position.y }
        ),
        .size = (vec2){ 0.0f, 0.0f }
    };

    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++) {
        luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);
        
        const cmrect2d bton_rect = (cmrect2d) {
            .position = bton->position,
            .size = bton->size
        };
        if (cmAABB(&mouse, &bton_rect)) {
            bton->was_hovered = 1;
            if (mouse_pressed && bton->pressed) {
                bton->was_pressed = 1;
                bton->pressed(bton);
            } else if (bton->hover) {
                bton->was_pressed = 0;
                bton->hover(bton);
            }
        } else {
            bton->was_hovered = 0;
            bton->was_pressed = 0;
        }
    }
}