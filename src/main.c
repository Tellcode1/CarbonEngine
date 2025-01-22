#include "../common/printf.h"
#include "../common/stdafx.h"
#include "../common/timer.h"
#include "../external/box2d/include/box2d/box2d.h"
#include "../include/engine/cengine.h"
#include "../include/engine/ctext.h"
#include "../include/engine/lunaCamera.h"
#include "../include/engine/lunaInput.h"
#include "../include/engine/lunaObject.h"
#include "../include/engine/lunaScene.h"
#include "../include/engine/lunaUI.h"

#include <math.h>
#include <stdio.h>

#include "../impact/include/impact.h"

#include "../common/cvar.h"
#include "../common/mem.h"

cvar *g_vars = NULL;
int g_nvars  = 0;

void leave_game(lunaUI_Button *self) {
  (void)self;
  luna_ApplicationRunning = 0;
}

void hoover(lunaUI_Button *self) {
  self->color = (vec4){0.6f, 0.6f, 0.6f, 1.0f};
}

void test_allocator() {
  int buffer[BUFSIZ];

  lunaStackAllocator stack;
  sainit(&stack, (unsigned char *)buffer, sizeof(buffer));

  lunaAllocator ac;
  lunaAllocatorBindStackAllocator(&ac, &stack);

  int *allocations[256];

  for (int i = 0; i < array_len(allocations); i++) {
    int *allocation = saalloc(&ac, sizeof(int));
    *allocation     = i;
    allocations[i]  = allocation;
  }

  bool pass = 1;
  for (int i = 0; i < array_len(allocations); i++) {
    if (i != *(allocations[i]))
      pass = 0;
  }

  if (!pass) {
    LOG_AND_ABORT("Test Failed");
  } else {
    LOG_INFO("Test passed");
  }

  for (int i = 0; i < array_len(allocations); i++) {
    safree(&ac, (void *)allocations[i]);
  }

  return;
}

int main(int argc, char *argv[]) {
  test_allocator();

  timer start_timer = timer_begin(0.1);

  (void)argc;
  (void)argv;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  const lunaExtent2D window_size = (lunaExtent2D){1600 / 400, 1200 / 400};

  cg_initialize_context("luna", window_size.width, window_size.height);

  lunaRenderer_Config rdconf  = lunaRenderer_ConfigInit();
  rdconf.vsync_enabled        = 0;
  rdconf.buffer_mode          = LUNA_BUFFER_MODE_TRIPLE_BUFFERED;
  rdconf.window_resizable     = 0;
  rdconf.initial_window_size  = window_size;
  rdconf.multisampling_enable = 0;
  rdconf.samples              = LUNA_SAMPLE_COUNT_1_SAMPLES;
  lunaRenderer_t *rd          = lunaRenderer_Init(&rdconf);

  // If you're wondering why every Action has a +,
  // I want to create a resource system with info about everything like bindings
  // It'll be used to serialize a save, for example
  // and etc. and every event will have a preceding +, booleans will have a 0 and integers will have an i
  // I'll drop the + (the user won't have to add it) when i get to it

  lunaInput_Init();
  lunaInput_BindKeyToAction(SDL_SCANCODE_SPACE, "+jump");
  lunaInput_BindKeyToAction(SDL_SCANCODE_RIGHT, "+right");
  lunaInput_BindKeyToAction(SDL_SCANCODE_LEFT, "+left");
  lunaInput_BindKeyToAction(SDL_SCANCODE_RETURN, "+Enter");

  const float updateTime = 15.0f; // seconds. 1.5f = 1.5 seconds
  float totalTime        = 0.0f;
  u32 numFrames          = 0;

  cfont_t *amongus;
  ctext_load_font(rd, "./bakedfont", 256.0f, &amongus);

  int curr_showing_fps = 0;

  float accumulator    = 0.0f;
  const float timeStep = 1.0f / 60.0f;

  lunaUI_Init();

  lunaUI_Button *bton    = lunaUI_CreateButton(lunaSprite_LoadFromDisk("../Assets/x.png"));
  bton->transform.size.x = 5.0f;
  bton->transform.size.y = 5.0f;
  bton->on_click         = leave_game;
  bton->on_hover         = hoover;

  lunaScene *scn = lunaScene_Init();

  const vec2 r1pos = (vec2){0.0f, 10.0f};
  const vec2 r2pos = (vec2){0.0f, -3.0f};

  const vec2 r1siz = (vec2){1.0f, 1.0f};
  const vec2 r2siz = (vec2){10.0f, 1.0f};

  lunaObject *r1 = lunaObject_Create(scene_main, "Rect1", LUNA_COLLIDER_TYPE_DYNAMIC, B2_DEFAULT_CATEGORY_BITS, B2_DEFAULT_MASK_BITS, r1pos,
                                     v2muls(r1siz, 2.0f), LUNA_OBJECT_NO_COLLISION);

  lunaObject *r2 = lunaObject_Create(scene_main, "Rect2", LUNA_COLLIDER_TYPE_DYNAMIC, B2_DEFAULT_CATEGORY_BITS, B2_DEFAULT_MASK_BITS, r2pos,
                                     v2muls(r2siz, 2.0f), LUNA_OBJECT_NO_COLLISION);
  lunaObject_GetSpriteRenderer(r1)->color = (vec4){1.0f, 0.0f, 0.0f, 1.0f};
  lunaObject_GetSpriteRenderer(r2)->color = (vec4){0.0f, 0.0f, 1.0f, 1.0f};

  IWorld world = {.gravity = (vec3){0.0f, -10.0f, 0.0f}};

  IBodyCreateInfo ibodyinfo = {};
  ibodyinfo.type            = IMPACT_BODY_DYNAMIC;
  ibodyinfo.position        = (vec3){r1pos.x, r1pos.y, 0.0f};
  ibodyinfo.half_size       = (vec3){r1siz.x, r1siz.y, 1.0f};
  ibodyinfo.restitution     = 0.3f;
  IBody *r1b                = IBodyCreate(&world, &ibodyinfo);

  ibodyinfo.position       = (vec3){r2pos.x, r2pos.y, 0.0f};
  ibodyinfo.half_size      = (vec3){r2siz.x, r2siz.y, 1.0f};
  ibodyinfo.start_disabled = 1;
  ibodyinfo.type           = IMPACT_BODY_STATIC;
  IBody *r2b               = IBodyCreate(&world, &ibodyinfo);

  // What in the unholy f%$ where you doing
  LOG_DEBUG("Initialized in %li ms (%.3f s)", (long)(timer_time_since_start(&start_timer) * 1000.0), timer_time_since_start(&start_timer));
  while (cg_running()) {
    cg_update();
    const double dt = cg_get_delta_time();

    if (lunaInput_IsActionJustSignalled("+jump")) {
      IBodyMove(r1b, (vec3){0.0f, 5.0f, 0.0f});
    }
    if (lunaInput_IsActionSignalled("+left")) {
      IBodyMove(r1b, (vec3){25.0f * -dt, 0.0f, 0.0f});
    }
    if (lunaInput_IsActionSignalled("+right")) {
      IBodyMove(r1b, (vec3){25.0f * dt, 0.0f, 0.0f});
    }

    accumulator += dt;
    while (accumulator >= timeStep) {
      // fixed update

      float prevpos = r2b->rect.position.y;

      IWorldStep(&world, 1.0 / 60.0);

      r2b->rect.position.y = prevpos;

      lunaObject_SetPosition(r1, (vec2){r1b->rect.position.x, r1b->rect.position.y});
      lunaObject_SetPosition(r2, (vec2){r2b->rect.position.x, r2b->rect.position.y});

      lunaScene_Update();
      accumulator -= timeStep;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      cg_consume_event(&event);
    }
    lunaInput_Update();

    // Profiling code
    totalTime += dt;
    numFrames++;
    if (totalTime >= updateTime) {
      curr_showing_fps = ceilf(numFrames / totalTime);
      LOG_INFO("%i FPS %f MS/Frame", curr_showing_fps, (totalTime / (float)(numFrames)));
      numFrames = 0;
      totalTime = 0.0;
    }

    lunaCamera_Update(&camera, rd);

    lunaUI_Update();
    if (!bton->was_hovered) { // If the mouse is NOT on the button, change it
                              // back to it's default color
      bton->color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
    }
    bton->transform.position = v2add((vec2){camera.position.x, camera.position.y}, (vec2){-9.0f, 9.0f});
    bton->transform.size     = (vec2){1.0f, 1.0f};

    lunaRenderer_SetClearColor(rd, (vec4){.x = 0.2f, .y = 0.2f, .z = 0.2f, .w = 1.0f});

    if (lunaRenderer_BeginRender(rd)) {

      lunaScene_Render(rd);

      lunaRenderer_EndRender(rd);
    }
  }

  // Destroying everything isn't that performance intensive...
  // oh wait I was using a global stack allocator...
  timer finish_timer = timer_begin(0.1);

  lunaScene_Destroy(scn);

  lunaUI_DestroyButton(bton);

  vkDeviceWaitIdle(device);
  ctext_destroy_font(amongus);
  lunaUI_Shutdown();
  ctext_shutdown(rd);
  lunaRenderer_Destroy(rd);
  lunaInput_Shutdown();
  LOG_INFO("done");
  LOG_INFO("Took %f seconds to clean up", timer_time_since_start(&finish_timer));
  return 0;
}
