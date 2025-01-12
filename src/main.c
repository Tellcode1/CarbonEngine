#include "../include/common/printf.h"
#include "../include/common/stdafx.h"
#include "../include/common/timer.h"
#include "../include/engine/cengine.h"
#include "../include/engine/ctext.h"
#include "../include/engine/lunaCamera.h"
#include "../include/engine/lunaInput.h"
#include "../include/engine/lunaObject.h"
#include "../include/engine/lunaScene.h"
#include "../include/engine/lunaUI.h"
#include "box2d/types.h"

#include <math.h>
#include <stdio.h>

static size_t cookie_count = 0;

void pressed(lunaUI_Button *self) {
  (void)self;
  ++cookie_count;
}

void leave_game(lunaUI_Button *self) {
  (void)self;
  cg_application_running = 0;
}

void hoover(lunaUI_Button *self) {
  self->color = (vec4){0.6f, 0.6f, 0.6f, 1.0f};
}

int main(int argc, char *argv[]) {
  timer start_timer = timer_begin(0.1);

  (void)argc;
  (void)argv;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  const lunaExtent2D window_size = (lunaExtent2D){800, 600};

  cg_initialize_context("luna", window_size.width, window_size.height);

  lunaRenderer_Config rdconf = lunaRenderer_ConfigInit();
  rdconf.vsync_enabled = 0;
  rdconf.buffer_mode = LUNA_BUFFER_MODE_TRIPLE_BUFFERED;
  rdconf.window_resizable = 0;
  rdconf.initial_window_size = window_size;
  rdconf.multisampling_enable = 0;
  rdconf.samples = LUNA_SAMPLE_COUNT_1_SAMPLES;
  lunaRenderer_t *rd = lunaRenderer_Init(&rdconf);

  lunaInput_Init();
  lunaInput_BindKeyToAction(SDL_SCANCODE_SPACE, "+jump");
  lunaInput_BindKeyToAction(SDL_SCANCODE_RIGHT, "+right");
  lunaInput_BindKeyToAction(SDL_SCANCODE_LEFT, "+left");

  const float updateTime = 5.0f; // seconds. 1.5f = 1.5 seconds
  float totalTime = 0.0f;
  u32 numFrames = 0;

  cfont_t *amongus;
  ctext_load_font(rd, "./bakedfont", 256.0f, &amongus);

  int curr_showing_fps = 0;

  float accumulator = 0.0f;
  const float timeStep = 1.0f / 60.0f;

  lunaUI_Init();

  lunaUI_Button *bton =
      lunaUI_CreateButton(lunaSprite_LoadFromDisk("../Assets/x.png"));
  bton->transform.size.x = 5.0f;
  bton->transform.size.y = 5.0f;
  bton->on_click = leave_game;
  bton->on_hover = hoover;

  lunaScene *scn = lunaScene_Init();

  lunaObject *player =
      lunaObject_Create(scene_main, "Player", LUNA_COLLIDER_TYPE_DYNAMIC,
                        B2_DEFAULT_CATEGORY_BITS, B2_DEFAULT_MASK_BITS,
                        (vec2){0.0f, 0.0f}, (vec2){1.0f, 1.0f}, 0);
  lunaObject_GetSpriteRenderer(player)->spr = lunaSprite_LoadFromDisk("../Assets/circle.png");

  lunaObject *ground = lunaObject_Create(
      scene_main, "Ground", LUNA_COLLIDER_TYPE_STATIC, B2_DEFAULT_CATEGORY_BITS,
      B2_DEFAULT_MASK_BITS, (vec2){0.0f, -12.0f}, (vec2){10.0f, 1.0f}, 0);
  (void)ground;

  ctext_label *l = ctext_create_label(scene_main, amongus);

  // What in the unholy f%$ where you doing
  LOG_DEBUG("Initialized in %li ms (%.3f s)",
            (long)(timer_time_since_start(&start_timer) * 1000.0),
            timer_time_since_start(&start_timer));
  while (cg_running()) {
    cg_update();
    const double dt = cg_get_delta_time();

    accumulator += dt;
    while (accumulator >= timeStep) {
      // fixed update
      lunaScene_Update();
      accumulator -= timeStep;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      cg_consume_event(&event);
    }
    lunaInput_Update();

    const float speed = 25.0f;
    if (lunaInput_IsActionSignalled("+left")) {
      lunaObject_Move(player, (vec2){-speed * cg_get_delta_time(), 0.0f});
    }
    if (lunaInput_IsActionSignalled("+right")) {
      lunaObject_Move(player, (vec2){speed * cg_get_delta_time(), 0.0f});
    }
    if (lunaInput_IsActionJustSignalled("+jump")) {
      lunaObject_Move(player, (vec2){0.0f, 5.0f});
    }

    vec2 pos = lunaObject_GetPosition(player);
    lunaCamera_SetPosition(&camera, (vec3){pos.x, pos.y, camera.position.z});

    // Profiling code
    totalTime += dt;
    numFrames++;
    if (totalTime >= updateTime) {
      curr_showing_fps = ceilf(numFrames / totalTime);
      LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps,
               numFrames, updateTime, (totalTime / (float)(numFrames)));
      numFrames = 0;
      totalTime = 0.0;
    }

    lunaCamera_Update(&camera, rd);

    lunaUI_Update();
    if (!bton->was_hovered) { // If the mouse is NOT on the button, change it
                              // back to it's default color
      bton->color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
    }
    bton->transform.position = v2add(
        (vec2){camera.position.x, camera.position.y}, (vec2){-9.0f, 9.0f});
    bton->transform.size = (vec2){1.0f, 1.0f};

    ctext_label_set_text(l, lunaObject_GetName(player));
    lunaObject *obj = ctext_label_get_object(l);
    lunaObject_SetPosition(obj, lunaObject_GetPosition(player));

    lunaRenderer_SetClearColor(rd, (vec4){.x=0.5f*(sinf(cg_get_time()) + 1.0f), .y=0.3f, .z=0.2f, .w=1.0f});

    if (lunaRenderer_BeginRender(rd)) {

      lunaScene_Render(rd);

      lunaRenderer_EndRender(rd);
    }
  }

  lunaScene_Destroy(scn);

  lunaUI_DestroyButton(bton);

  vkDeviceWaitIdle(device);
  lunaSprite_Release(lunaObject_GetSpriteRenderer(player)->spr);
  ctext_destroy_font(amongus);
  lunaRenderer_Destroy(rd);
  lunaInput_Shutdown();
  LOG_INFO("done.");
  return 0;
}
