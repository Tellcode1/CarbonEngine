#ifndef __NV_INPUT_H__
#define __NV_INPUT_H__

#include "../../common/math/vec2.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>

NOVA_HEADER_START;

// Only one key OR mouse button may be bound to an action currently!

typedef struct NV_input_action NV_input_action;

// A function that is called every time an action is signalled
// This is better than polling the event every frame when the action is signalled multiple times per frame
typedef void (*NV_input_action_response_fn)(const char* action, const NV_input_action* ia);

typedef enum NV_input_action_type
{
  NOVA_INPUT_ACTION_TYPE_KEYBOARD = 1,
  NOVA_INPUT_ACTION_TYPE_MOUSE    = 2,
  NOVA_INPUT_ACTION_TYPE_ALL      = (1 | 2)
} NV_input_action_type;

typedef enum NV_input_key_state
{
  NOVA_KEY_STATE_PRESSED  = 0,
  NOVA_KEY_STATE_RELEASED = 1,
  NOVA_KEY_STATE_HELD     = 2,
  NOVA_KEY_STATE_NOT_HELD = 3,
} NV_input_key_state;

typedef enum NV_input_mouse_button
{
  NOVA_MOUSE_BUTTON_LEFT   = 0,
  NOVA_MOUSE_BUTTON_MIDDLE = 1,
  NOVA_MOUSE_BUTTON_RIGHT  = 2,
} NV_input_mouse_button;

struct NV_input_action
{
  SDL_Scancode                key;
  SDL_Scancode                key2;
  uint8_t                     mouse;
  NV_input_action_response_fn response;
  bool                        this_frame, last_frame;
};

extern void NV_input_init();
extern void NV_input_update();
extern void NV_input_shutdown();

void        NV_input_bind_function_to_action(const char* action, NV_input_action_response_fn response);

extern void NV_input_bind_key_to_action(SDL_Scancode key, const char* action);
extern void NV_input_bind_mouse_to_action(int bton, const char* action);

// remove all keys, mouse buttons and the response function from action
// essentially, clear it.
extern void NV_input_unbind_action(const char* action);

// action held
extern bool NV_input_is_action_signalled(const char* action);

// action pressed
extern bool NV_input_is_action_just_signalled(const char* action);

// action not held
extern bool NV_input_is_action_unsignalled(const char* action);

// action released
extern bool NV_input_is_action_just_unsignalled(const char* action);

/// @brief signals the action for a frame
/// @return 0 on action signalled, -1 if it can't find the action specified.
extern int                NV_input_signal_action(const char* action);

extern NV_input_key_state NV_input_get_key_state(const SDL_Scancode sc);
extern bool               NV_input_is_key_signalled(const SDL_Scancode sc);
extern bool               NV_input_is_key_unsignalled(const SDL_Scancode sc);
extern bool               NV_input_is_key_just_signalled(const SDL_Scancode sc);
extern bool               NV_input_is_key_just_unsignalled(const SDL_Scancode sc);

extern vec2               NV_input_get_mouse_position(void);
extern vec2               NV_input_get_last_frame_mouse_position(void);
extern vec2               NV_input_get_mouse_delta(void);

// button is 1 for left mouse, 2 for middle, 3 for right
extern bool NV_input_is_mouse_signalled(NV_input_mouse_button button);
extern bool NV_input_is_mouse_just_signalled(NV_input_mouse_button button);

NOVA_HEADER_END;

#endif //__NOVA_INPUT_H__