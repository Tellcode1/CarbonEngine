#include "cinput.hpp"

std::bitset<SDL_NUM_SCANCODES> cinput::kb_state;
std::bitset<SDL_NUM_SCANCODES> cinput::last_frame_kb_state;
vec2 cinput::mouse_position;