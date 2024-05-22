#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include "stdafx.hpp"
#include <bitset>

enum KeyState {
    KEY_RELEASED = 0,
    KEY_PRESSED = 1,
    KEY_HELD = 2,
};

struct KeyboardEvent {
    SDL_Keycode keycode;
    KeyState type;  // SDL_KEYDOWN or SDL_KEYUP
};

struct MouseEvent {
    f32 x, y;
    u8 type;  // SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP
    u8 button;
};

struct ControllerEvent {
    SDL_JoystickID id;
    u8 type;
    SDL_GamepadButton button;
};

// static InputSingleton* Input = InputSingleton::GetSingleton();

#endif