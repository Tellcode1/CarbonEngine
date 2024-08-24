#include "../include/cinput.hpp"

cbitset<SDL_NUM_SCANCODES> cinput::kb_state;
cbitset<SDL_NUM_SCANCODES> cinput::last_frame_kb_state;
vec2 cinput::mouse_position;
vec2 cinput::last_frame_mouse_position;