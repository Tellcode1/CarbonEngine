#ifndef __LUNA_INPUT_H__
#define __LUNA_INPUT_H__

#include "../../common/math/vec2.h"
#include <SDL2/SDL.h>

// Only one key OR mouse button may be bound to an action currently!

typedef struct lunaInput_Action lunaInput_Action;

typedef enum lunaInput_ActionType lunaInput_ActionType;
typedef enum lunaInput_KeyState lunaInput_KeyState;
typedef enum lunaInput_MouseButton lunaInput_MouseButton;

// A function that is called every time an action is signalled
// This is better than polling the event every frame when the action is signalled multiple times per frame
typedef void (*lunaInput_ActionResponseFn)(const char *action, const lunaInput_Action *ia);

enum lunaInput_ActionType { LUNA_INPUT_ACTION_TYPE_KEYBOARD = 1, LUNA_INPUT_ACTION_TYPE_MOUSE = 2, LUNA_INPUT_ACTION_TYPE_ALL = (1 | 2) };

enum lunaInput_KeyState {
  LUNA_KEY_STATE_PRESSED  = 0,
  LUNA_KEY_STATE_RELEASED = 1,
  LUNA_KEY_STATE_HELD     = 2,
  LUNA_KEY_STATE_NOT_HELD = 3,
};

enum lunaInput_MouseButton {
  LUNA_MOUSE_BUTTON_LEFT   = 0,
  LUNA_MOUSE_BUTTON_MIDDLE = 1,
  LUNA_MOUSE_BUTTON_RIGHT  = 2,
};

struct lunaInput_Action {
  SDL_Scancode key; SDL_Scancode key2;
  uint8_t mouse;
  lunaInput_ActionResponseFn response;
  bool this_frame, last_frame;
};

extern void lunaInput_Init();
extern void lunaInput_Update();
extern void lunaInput_Shutdown();

void lunaInput_BindFunctionToAction(const char *action, lunaInput_ActionResponseFn response);

extern void lunaInput_BindKeyToAction(SDL_Scancode key, const char *action);
extern void lunaInput_BindMouseToAction(int bton, const char *action);

// Remove all keys, mouse buttons and the response function from action
// essentially, clear it.
extern void lunaInput_UnbindAction(const char *action);

// Action held
extern bool lunaInput_IsActionSignalled(const char *action);

// Action pressed
extern bool lunaInput_IsActionJustSignalled(const char *action);

// Action not held
extern bool lunaInput_IsActionUnsignalled(const char *action);

// Action released
extern bool lunaInput_IsActionJustUnsignalled(const char *action);

/// @brief Signals the action for a frame
/// @return 0 on action signalled, -1 if it can't find the action specified.
extern int lunaInput_SignalAction(const char *action);

extern lunaInput_KeyState lunaInput_GetKeyState(const SDL_Scancode sc);
extern bool lunaInput_IsKeySignalled(const SDL_Scancode sc);
extern bool lunaInput_IsKeyUnsignalled(const SDL_Scancode sc);
extern bool lunaInput_IsKeyJustSignalled(const SDL_Scancode sc);
extern bool lunaInput_IsKeyJustUnsignalled(const SDL_Scancode sc);

extern vec2 lunaInput_GetMousePosition(void);
extern vec2 lunaInput_GetLastFrameMousePosition(void);
extern vec2 lunaInput_GetMouseDelta(void);

// button is 1 for left mouse, 2 for middle, 3 for right
extern bool lunaInput_IsMouseSignalled(lunaInput_MouseButton button);
extern bool lunaInput_IsMouseJustSignalled(lunaInput_MouseButton button);

#endif //__LUNA_INPUT_H__