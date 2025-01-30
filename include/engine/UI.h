#ifndef __NV_UI_H
#define __NV_UI_H

#include "../../common/math/vec4.h"
#include "object.h"
#include "sprite.h"

NOVA_HEADER_START;

typedef struct NVUI_Button NVUI_Button;
typedef struct NVUI_Slider NVUI_Slider;

typedef void (*NVUI_ButtonOnClick)(NVUI_Button *bton);
typedef void (*NVUI_ButtonOnHover)(NVUI_Button *bton);

extern struct NVUI_Context NV_ui_ctx;

struct NVUI_Button {
  NVTransform transform;
  vec4 color;
  NVUI_ButtonOnHover on_hover;
  NVUI_ButtonOnClick on_click;
  NVSprite *spr;
  bool was_hovered; // was it being hovered in this frame?
  bool was_clicked; // was the button pressed?
};

struct NVUI_Slider {
  NVTransform transform;
  vec4 bg_color, slider_color;
  NVSprite *bg_sprite, *slider_sprite;
  float min, max, value;
  bool moved;        // was the slider's handle moved
  bool interactable; // whether this slider can be controlled by the mouse.
                     // default off
};

extern void NVUI_Init();
extern void NVUI_Shutdown();

extern NVUI_Button *NVUI_CreateButton(struct NVSprite *spr);

extern NVUI_Slider *NVUI_CreateSlider();

extern void NVUI_DestroyButton(NVUI_Button *obj);
extern void NVUI_DestroySlider(NVUI_Slider *obj);

extern void NVUI_Render(NVRenderer_t *rd);

extern void NVUI_Update();

NOVA_HEADER_END;

#endif //__NOVA_UI_H