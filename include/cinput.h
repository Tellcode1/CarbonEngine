#ifndef __C_INPUT_HPP__
#define __C_INPUT_HPP__

#ifdef __cplusplus
    extern "C" {
#endif

// !!!! Key presses are NOT working because cbitset_set_bit_to blindly sets bit value. FIX!

#include <SDL2/SDL.h>
#include "../include/stdafx.h"
#include "../include/cgbitset.h"
#include "../include/math/vec2.h"

typedef enum key_state
{
    KB_STATE_PRESSED = 0,
    KB_STATE_RELEASED = 1,
    KB_STATE_HELD = 2,
    KB_STATE_NOT_HELD = 3,
    KB_STATE_INVALID = __INT32_MAX__
} key_state;

extern vec2 mouse_position;
extern vec2 last_frame_mouse_position;
extern cg_bitset_t *cinput_kb_state;
extern cg_bitset_t *cinput_last_frame_kb_state;

static inline void cinput_init() {
    cinput_kb_state = cg_bitset_init( SDL_NUM_SCANCODES );
    cinput_last_frame_kb_state = cg_bitset_init( SDL_NUM_SCANCODES );
    mouse_position = (vec2){};
    last_frame_mouse_position = (vec2){};
}

static inline void cinput_update() {
    // i32 mx, my;
    // SDL_GetMouseState(&mx, &my);

    // * do we actually have to make renderer a parameter to cinput
    // const f32 width = (f32)vctx::RenderExtent.width;
    // const f32 height = (f32)vctx::RenderExtent.height;

    // last_frame_mouse_position = mouse_position;
    // mouse_position.x = (f32(mx) / width)  * 2.0f - 1.0f;
    // mouse_position.y = (f32(my) / height) * 2.0f - 1.0f;

    const u8 *const sdl_kb_state = SDL_GetKeyboardState(NULL);
    cinput_last_frame_kb_state = cinput_kb_state;
    for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
        cg_bitset_set_bit_to(cinput_kb_state, i, sdl_kb_state[i]);
}

static inline key_state cinput_get_key_state(const SDL_Scancode sc) {
    const bool key_state = cg_bitset_access_bit(cinput_kb_state, sc);
    const bool last_frame_key_state = cg_bitset_access_bit(cinput_last_frame_kb_state, sc);

    // curly braces are beautiful, aren't they?
    if (key_state && last_frame_key_state) {
        return KB_STATE_HELD;
    } else if (!key_state && !last_frame_key_state) {
        return KB_STATE_NOT_HELD;
    } else if (key_state && !last_frame_key_state) {
        return KB_STATE_PRESSED;
    } else if (!key_state && last_frame_key_state) {
        return KB_STATE_RELEASED;
    }

    return KB_STATE_INVALID;
}

static inline bool cinput_is_key_held(const SDL_Scancode sc) {
    return cinput_get_key_state(sc) == KB_STATE_HELD;
}

static inline bool cinput_is_key_not_held(const SDL_Scancode sc) {
    return cinput_get_key_state(sc) == KB_STATE_NOT_HELD;
}

static inline bool cinput_is_key_pressed(const SDL_Scancode sc) {
    return cinput_get_key_state(sc) == KB_STATE_PRESSED;
}

static inline bool cinput_is_key_released(const SDL_Scancode sc) {
    return cinput_get_key_state(sc) == KB_STATE_RELEASED;
}

#ifdef __cplusplus
    }
#endif

#endif