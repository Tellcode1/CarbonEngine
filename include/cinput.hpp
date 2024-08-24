#ifndef __C_INPUT_HPP__
#define __C_INPUT_HPP__

#include "../include/vkcb/vkcbstdafx.hpp"
#include "../include/math/vec2.h"
#include "../include/containers/cbitset.hpp"

struct cinput
{
    private:
    static cbitset<SDL_NUM_SCANCODES> kb_state;
    static cbitset<SDL_NUM_SCANCODES> last_frame_kb_state;

    static vec2 mouse_position;
    static vec2 last_frame_mouse_position;

    public:
    enum key_state : int
    {
        KB_STATE_PRESSED = 0,
        KB_STATE_RELEASED = 1,
        KB_STATE_HELD = 2,
        KB_STATE_NOT_HELD = 3,
        KB_STATE_INVALID = __INT32_MAX__
    };
    
    static void CARBON_FORCE_INLINE initialize() {
        mouse_position = vec2(0.0f, 0.0f);
        last_frame_mouse_position = vec2(0.0f, 0.0f);
    }

    static vec2 CARBON_FORCE_INLINE get_mouse_position() {
        return mouse_position;
    }

    static vec2 CARBON_FORCE_INLINE get_last_frame_mouse_position() {
        return last_frame_mouse_position;
    }

    static void CARBON_FORCE_INLINE update() {
        i32 mx, my;
        SDL_GetMouseState(&mx, &my);

        const f32 width = (f32)vctx::RenderExtent.width;
        const f32 height = (f32)vctx::RenderExtent.height;

        last_frame_mouse_position = mouse_position;
        mouse_position.x = (f32(mx) / width)  * 2.0f - 1.0f;
        mouse_position.y = (f32(my) / height) * 2.0f - 1.0f;

        const u8 *const sdl_kb_state = SDL_GetKeyboardState(NULL);
        last_frame_kb_state = kb_state;
        for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
            kb_state.set_value(i, sdl_kb_state[i]);
    }

    static bool CARBON_FORCE_INLINE is_key_held(const SDL_Scancode sc) {
        return get_key_state(sc) == KB_STATE_HELD;
    }

    static bool CARBON_FORCE_INLINE is_key_not_held(const SDL_Scancode sc) {
        return get_key_state(sc) == KB_STATE_NOT_HELD;
    }

    static bool CARBON_FORCE_INLINE is_key_pressed(const SDL_Scancode sc) {
        return get_key_state(sc) == KB_STATE_PRESSED;
    }

    static bool CARBON_FORCE_INLINE is_key_released(const SDL_Scancode sc) {
        return get_key_state(sc) == KB_STATE_RELEASED;
    }

    static key_state get_key_state(const SDL_Scancode sc) {
        const bool key_state = kb_state.test(sc);
        const bool last_frame_key_state = last_frame_kb_state.test(sc);
        if (key_state && last_frame_key_state)
            return KB_STATE_HELD;

        else if (!key_state && !last_frame_key_state)
            return KB_STATE_NOT_HELD;

        else if (key_state && !last_frame_key_state)
            return KB_STATE_PRESSED;

        else if (!key_state && last_frame_key_state)
            return KB_STATE_RELEASED;

        return KB_STATE_INVALID;
    }
};

#endif