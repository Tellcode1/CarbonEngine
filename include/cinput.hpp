#ifndef __C_INPUT_HPP__
#define __C_INPUT_HPP__

#include "vkcbstdafx.hpp"
#include <bitset>

struct cinput
{
    private:
    static std::bitset<SDL_NUM_SCANCODES> kb_state;
    static std::bitset<SDL_NUM_SCANCODES> last_frame_kb_state;

    static vec2 mouse_position;

    public:
    enum key_state : i32
    {
        KB_STATE_PRESSED = 0,
        KB_STATE_RELEASED = 1,
        KB_STATE_HELD = 2,
        KB_STATE_NOT_HELD = 3,
        KB_STATE_INVALID = __INT32_MAX__
    };
    
    static void CARBON_FORCE_INLINE initialize() {
        mouse_position = vec2(0.0f, 0.0f);
    }

    static vec2 CARBON_FORCE_INLINE get_mouse_position() {
        return mouse_position;
    }

    static void CARBON_FORCE_INLINE update() {
        i32 mx, my;
        SDL_GetMouseState(&mx, &my);
        mouse_position.x = ( f32(mx) / vctx::RenderExtent.width  ) * 2.0f - 1.0f;
        mouse_position.y = ( f32(my) / vctx::RenderExtent.height ) * 2.0f - 1.0f;

        const u8 * const sdl_kb_state = SDL_GetKeyboardState(NULL);
        last_frame_kb_state = kb_state;
        for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
            kb_state[i] = sdl_kb_state[i];
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
        if (kb_state[sc] && last_frame_kb_state[sc])
            return key_state::KB_STATE_HELD;

        else if (!kb_state[sc] && !last_frame_kb_state[sc])
            return key_state::KB_STATE_NOT_HELD;

        else if (kb_state[sc] && !last_frame_kb_state[sc])
            return key_state::KB_STATE_PRESSED;

        else if (!kb_state[sc] && last_frame_kb_state[sc])
            return key_state::KB_STATE_RELEASED;

        return KB_STATE_INVALID;
    }
};

#endif