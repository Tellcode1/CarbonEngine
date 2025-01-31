#include "../common/stdafx.h"
#include "../common/timer.h"
#include "../external/box2d/include/box2d/box2d.h"
#include "../external/volk/volk.h"
#include "../include/engine/UI.h"
#include "../include/engine/camera.h"
#include "../include/engine/ctext.h"
#include "../include/engine/engine.h"
#include "../include/engine/input.h"
#include "../include/engine/object.h"
#include "../include/engine/scene.h"

#include <math.h>
#include <stdio.h>

#include "../common/cvar.h"
#include "../common/mem.h"

cvar* g_vars  = NULL;
int   g_nvars = 0;

void
leave_game(NVUI_Button* self)
{
  (void)self;
  NV_ApplicationRunning = 0;
}

void
hoover(NVUI_Button* self)
{
  self->color = (vec4){ 0.6f, 0.6f, 0.6f, 1.0f };
}

__attribute__((__used__, __noinline__)) void
test_allocator(void)
{
  uchar            buf[1024];

  NVAllocatorStack stack;
  NVAllocatorStackInit(&stack, buf, 1024);

  NVAllocator ac;
  NVAllocatorBindStackAllocator(&ac, &stack);

  size_t*         allocations[32];
  bool            pass                = 1;

  volatile uchar* TestLargeAllocation = ac.alloc(&ac, 1, 100);
  for (int i = 0; i < 100; i++)
  {
    TestLargeAllocation[i] = (uchar)rand();
  }
  if (!TestLargeAllocation)
  {
    NV_LOG_ERROR("Large allocation failed");
    pass = 0;
  }
  if (NV_memset((uchar*)TestLargeAllocation, 0, 100) == NULL)
  {
    NV_LOG_ERROR("Large allocation memset failed");
    pass = 0;
  }

  for (size_t i = 0; i < NV_arrlen(allocations); i++)
  {
    size_t* allocation = ac.alloc(&ac, 1, sizeof(size_t));
    if (!allocation)
    {
      pass           = 0;
      allocations[i] = NULL;
      NV_LOG_ERROR("allocation failed %d", i);
      continue;
    }
    *allocation    = i;
    allocations[i] = allocation;
  }

  for (size_t i = 0; i < NV_arrlen(allocations); i++)
  {
    if (allocations[i] == NULL)
    {
      NV_LOG_ERROR("NULL allocation at index %d", i);
      pass = 0;
      continue;
    }
    size_t test = *(allocations[i]);
    if (i != test)
    {
      NV_LOG_ERROR("addr %p (index %d) has incorrect data. expected %d got %d", allocations[i], i, i, test);
      pass = 0;
    }
    for (size_t j = 0; j < i; j++)
    {
      if (allocations[i] == allocations[j])
      {
        NV_LOG_ERROR("duplicate allocations at index %d and %d: %p", i, j, allocations[i]);
        pass = 0;
      }
    }
  }

  for (size_t i = 0; i < NV_arrlen(allocations); i++)
  {
    if (allocations[i] != NULL)
    {
      ac.free(&ac, (void*)(allocations[i]));
      allocations[i] = NULL;
    }
  }

  if (!pass)
  {
    NV_LOG_AND_ABORT("Test Failed");
  }
  else
  {
    NV_LOG_INFO("Test Passed");
  }
}

int
main(int argc, char* argv[])
{
  test_allocator();

  timer start_timer = timer_begin(0.1);

  (void)argc;
  (void)argv;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  const NV_extent2d window_size = (NV_extent2d){ 400, 400 };

  NV_initialize_context("NOVA example", window_size.width, window_size.height);

  NV_renderer_config rdconf   = NV_renderer_configInit();
  rdconf.vsync_enabled        = 0;
  rdconf.buffer_mode          = NOVA_BUFFER_MODE_TRIPLE_BUFFERED;
  rdconf.window_resizable     = 0;
  rdconf.initial_window_size  = window_size;
  rdconf.multisampling_enable = 0;
  rdconf.samples              = NOVA_SAMPLE_COUNT_1_SAMPLES;
  NV_renderer_t* rd            = NVRenderer_Init(&rdconf);

  // If you're wondering why every Action has a +,
  // I want to create a resource system with info about everything like bindings
  // It'll be used to serialize a save, for example
  // and etc. and every event will have a preceding +, booleans will have a 0 and integers will have an i
  // I'll drop the + (the user won't have to add it) when i get to it

  NV_input_init();
  NV_input_bind_key_to_action(SDL_SCANCODE_SPACE, "+jump");
  NV_input_bind_key_to_action(SDL_SCANCODE_RIGHT, "+right");
  NV_input_bind_key_to_action(SDL_SCANCODE_LEFT, "+left");
  NV_input_bind_key_to_action(SDL_SCANCODE_RETURN, "+Enter");

  const float updateTime = 3.0f; // seconds. 1.5f = 1.5 seconds
  float       totalTime  = 0.0f;
  u32         numFrames  = 0;

  cfont_t*    amongus;
  ctext_load_font(rd, "./bakedfont", 256.0f, &amongus);

  int         curr_showing_fps = 0;

  float       accumulator      = 0.0f;
  const float timeStep         = 1.0f / 60.0f;

  NVUI_Init();

  NVUI_Button* bton      = NVUI_CreateButton(NV_sprite_load_from_disk("../Assets/x.png"));
  bton->transform.size.x = 5.0f;
  bton->transform.size.y = 5.0f;
  bton->on_click         = leave_game;
  bton->on_hover         = hoover;

  NV_scene_t*   scn         = NV_scene_Init();

  const vec2 r1pos       = (vec2){ 0.0f, 10.0f };
  const vec2 r2pos       = (vec2){ 0.0f, 0.0f };

  const vec2 r1siz       = (vec2){ 0.5f, 0.5f };
  const vec2 r2siz       = v2muls(camera.ortho_size, 0.4);

  NVObject*  r1          = NVObject_Create(scene_main, "Rect1", NOVA_COLLIDER_TYPE_DYNAMIC, B2_DEFAULT_CATEGORY_BITS, B2_DEFAULT_MASK_BITS, r1pos, v2muls(r1siz, 2.0f), 0);

  NVObject*  r2          = NVObject_Create(scene_main, "Rect2", NOVA_COLLIDER_TYPE_STATIC, B2_DEFAULT_CATEGORY_BITS, B2_DEFAULT_MASK_BITS, r2pos, v2muls(r2siz, 2.0f), 0);
  NVObject_GetSpriteRenderer(r1)->color = (vec4){ 1.0f, 0.0f, 0.0f, 1.0f };
  NVObject_GetSpriteRenderer(r2)->color = (vec4){ 0.0f, 0.0f, 1.0f, 1.0f };

  // What in the unholy f%$ where you doing
  NV_LOG_DEBUG("Initialized in %li ms (%.3f s)", (long)(timer_time_since_start(&start_timer) * 1000.0), timer_time_since_start(&start_timer));
  while (NV_running())
  {
    NV_update();
    const double dt = NV_get_delta_time();

    if (NV_input_is_action_just_signalled("+jump"))
    {
      NVObject_Move(r1, (vec2){ 0.0f, 5.0f });
    }
    if (NV_input_is_action_signalled("+left"))
    {
      NVObject_Move(r1, (vec2){ 25.0f * -dt, 0.0f });
    }
    if (NV_input_is_action_signalled("+right"))
    {
      NVObject_Move(r1, (vec2){ 25.0f * dt, 0.0f });
    }

    accumulator += dt;
    while (accumulator >= timeStep)
    {
      // fixed update
      NV_scene_update();
      accumulator -= timeStep;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      NV_consume_event(&event);
    }
    NV_input_update();

    // Profiling code
    totalTime += dt;
    numFrames++;
    if (totalTime >= updateTime)
    {
      curr_showing_fps = ceilf(numFrames / totalTime);
      NV_LOG_INFO("%i FPS %f MS/Frame", curr_showing_fps, (totalTime / (float)(numFrames)));
      numFrames = 0;
      totalTime = 0.0;
    }

    NV_camera_update(&camera, rd);

    NVUI_Update();
    if (!bton->was_hovered)
    { // If the mouse is NOT on the button, change it
      // back to it's default color
      bton->color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    bton->transform.position = v2add((vec2){ camera.position.x, camera.position.y }, (vec2){ -9.0f, 9.0f });
    bton->transform.size     = (vec2){ 1.0f, 1.0f };

    NV_renderer_set_clear_color(rd, (vec4){ .x = 0.2f, .y = 0.2f, .z = 0.2f, .w = 1.0f });

    if (NV_renderer_begin(rd))
    {
      NV_scene_render(rd);

      NV_renderer_end(rd);
    }
  }

  // Destroying everything isn't that performance intensive...
  // oh wait I was using a global stack allocator...
  timer finish_timer = timer_begin(0.1);

  NV_scene_destroy(scn);

  NVUI_DestroyButton(bton);

  vkDeviceWaitIdle(device);
  ctext_destroy_font(amongus);
  NVUI_Shutdown();
  ctext_shutdown(rd);
  NVRenderer_Destroy(rd);
  NV_input_shutdown();
  NV_LOG_INFO("done");
  NV_LOG_INFO("Took %f seconds to clean up", timer_time_since_start(&finish_timer));
  return 0;
}
