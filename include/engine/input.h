#ifndef __NV_INPUT_H__
#define __NV_INPUT_H__

#include "../../common/math/vec2.h"
#include <SDL2/SDL.h>

NOVA_HEADER_START;

// Only one key OR mouse button may be bound to an action currently!

typedef struct NVInput_Action NVInput_Action;

// A function that is called every time an action is signalled
// This is better than polling the event every frame when the action is signalled multiple times per frame
typedef void (*NVInput_ActionResponseFn)(const char *action, const NVInput_Action *ia);

typedef enum NVInput_ActionType {
  NOVA_INPUT_ACTION_TYPE_KEYBOARD = 1,
  NOVA_INPUT_ACTION_TYPE_MOUSE    = 2,
  NOVA_INPUT_ACTION_TYPE_ALL      = (1 | 2)
} NVInput_ActionType;

typedef enum NVInput_KeyState {
  NOVA_KEY_STATE_PRESSED  = 0,
  NOVA_KEY_STATE_RELEASED = 1,
  NOVA_KEY_STATE_HELD     = 2,
  NOVA_KEY_STATE_NOT_HELD = 3,
} NVInput_KeyState;

typedef enum NVInput_MouseButton {
  NOVA_MOUSE_BUTTON_LEFT   = 0,
  NOVA_MOUSE_BUTTON_MIDDLE = 1,
  NOVA_MOUSE_BUTTON_RIGHT  = 2,
} NVInput_MouseButton;

struct NVInput_Action {
  SDL_Scancode key;
  SDL_Scancode key2;
  uint8_t mouse;
  NVInput_ActionResponseFn response;
  bool this_frame, last_frame;
};

extern void NVInput_Init();
extern void NVInput_Update();
extern void NVInput_Shutdown();

void NVInput_BindFunctionToAction(const char *action, NVInput_ActionResponseFn response);

extern void NVInput_BindKeyToAction(SDL_Scancode key, const char *action);
extern void NVInput_BindMouseToAction(int bton, const char *action);

// Remove all keys, mouse buttons and the response function from action
// essentially, clear it.
extern void NVInput_UnbindAction(const char *action);

// Action held
extern bool NVInput_IsActionSignalled(const char *action);

// Action pressed
extern bool NVInput_IsActionJustSignalled(const char *action);

// Action not held
extern bool NVInput_IsActionUnsignalled(const char *action);

// Action released
extern bool NVInput_IsActionJustUnsignalled(const char *action);

/// @brief Signals the action for a frame
/// @return 0 on action signalled, -1 if it can't find the action specified.
extern int NVInput_SignalAction(const char *action);

extern NVInput_KeyState NVInput_GetKeyState(const SDL_Scancode sc);
extern bool NVInput_IsKeySignalled(const SDL_Scancode sc);
extern bool NVInput_IsKeyUnsignalled(const SDL_Scancode sc);
extern bool NVInput_IsKeyJustSignalled(const SDL_Scancode sc);
extern bool NVInput_IsKeyJustUnsignalled(const SDL_Scancode sc);

extern vec2 NVInput_GetMousePosition(void);
extern vec2 NVInput_GetLastFrameMousePosition(void);
extern vec2 NVInput_GetMouseDelta(void);

// button is 1 for left mouse, 2 for middle, 3 for right
extern bool NVInput_IsMouseSignalled(NVInput_MouseButton button);
extern bool NVInput_IsMouseJustSignalled(NVInput_MouseButton button);

NOVA_HEADER_END;

#endif //__NOVA_INPUT_H__