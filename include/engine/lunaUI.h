#ifndef __LUNA_UI_H
#define __LUNA_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/math/vec4.h"
#include "lunaObject.h"
#include "lunaSprite.h"

typedef struct lunaUI_Button lunaUI_Button;
typedef struct lunaUI_Slider lunaUI_Slider;

typedef void (*lunaUI_ButtonOnClick)(lunaUI_Button *bton);
typedef void (*lunaUI_ButtonOnHover)(lunaUI_Button *bton);

extern struct lunaUI_Context luna_ui_ctx;

struct lunaUI_Button {
  lunaTransform transform;
  vec4 color;
  lunaUI_ButtonOnHover on_hover;
  lunaUI_ButtonOnClick on_click;
  lunaSprite *spr;
  bool was_hovered; // was it being hovered in this frame?
  bool was_clicked; // was the button pressed?
};

struct lunaUI_Slider {
  lunaTransform transform;
  vec4 bg_color, slider_color;
  lunaSprite *bg_sprite, *slider_sprite;
  float min, max, value;
  bool moved;        // was the slider's handle moved
  bool interactable; // whether this slider can be controlled by the mouse.
                     // default off
};

extern void lunaUI_Init();
extern void lunaUI_Shutdown();

extern lunaUI_Button *lunaUI_CreateButton(struct lunaSprite *spr);

extern lunaUI_Slider *lunaUI_CreateSlider();

extern void lunaUI_DestroyButton(lunaUI_Button *obj);
extern void lunaUI_DestroySlider(lunaUI_Slider *obj);

extern void lunaUI_Render(lunaRenderer_t *rd);

extern void lunaUI_Update();

#ifdef __cplusplus
}
#endif

#endif //__LUNA_UI_H