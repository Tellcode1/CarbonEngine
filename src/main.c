#include "../include/common/stdafx.h"
#include "../include/common/timer.h"
#include "../include/engine/cengine.h"
#include "../include/engine/ctext.h"
#include "../include/engine/lunaCamera.h"
#include "../include/engine/lunaInput.h"
#include "../include/engine/lunaObject.h"
#include "../include/engine/lunaScene.h"
#include "../include/engine/lunaUI.h"

#include <math.h>

static size_t cookie_count       = 0;
static float  cookies_per_second = 9.0f;

void pressed(luna_UI_Button* self)
{
  (void) self;
  ++cookie_count;
}

void leave_game(luna_UI_Button* self)
{
  (void) self;
  cg_application_running = 0;
}

void hoover(luna_UI_Button* self)
{
  self->color = (vec4){0.6f, 0.6f, 0.6f, 1.0f};
}

int main(int argc, char* argv[])
{
  (void) argc;
  (void) argv;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

  const lunaExtent2D window_size = (lunaExtent2D){800, 600};

  cg_initialize_context("luna", window_size.width, window_size.height);

  lunaRenderer_Config rdconf  = crender_config_init();
  rdconf.vsync_enabled        = 0;
  rdconf.buffer_mode          = CG_BUFFER_MODE_TRIPLE_BUFFERED;
  rdconf.window_resizable     = 0;
  rdconf.initial_window_size  = window_size;
  rdconf.multisampling_enable = 0;
  rdconf.samples              = CG_SAMPLE_COUNT_1_SAMPLES;
  lunaRenderer_t* rd          = lunaRenderer_Init(&rdconf);

  lunaInput_Init();
  lunaInput_BindKeyToAction(SDL_SCANCODE_SPACE, "+jump");
  lunaInput_BindKeyToAction(SDL_SCANCODE_RIGHT, "+right");
  lunaInput_BindKeyToAction(SDL_SCANCODE_LEFT, "+left");

  const float updateTime = 5.0f; // seconds. 1.5f = 1.5 seconds
  float       totalTime  = 0.0f;
  u32         numFrames  = 0;

  cfont_t* amongus;
  ctext_load_font(rd, "./bakedfont", 256.0f, &amongus);

  int curr_showing_fps = 0;

  float       accumulator = 0.0f;
  const float timeStep    = 1.0f / 60.0f;

  luna_UI_Init();

  luna_UI_Button* bton = luna_UI_CreateButton(lunaSprite_LoadFromDisk("../Assets/x.png"));
  bton->size.x         = 5.0f;
  bton->size.y         = 5.0f;
  bton->pressed        = leave_game;
  bton->hover          = hoover;

  lunaScene* scn = lunaScene_Init();

  lunaScene_ChangeToScene(scn);

  lunaObject* player = lunaObject_Create(scene_main, "Player", LUNA_COLLIDER_TYPE_DYNAMIC, (vec2){},
                                         (vec2){1.0f, 1.0f}, 0);

  lunaObject* ground = lunaObject_Create(scene_main, "Player", LUNA_COLLIDER_TYPE_STATIC,
                                         (vec2){0.0f, -2.0f}, (vec2){10.0f, 1.0f}, 0);
  (void) ground;

  timer cookie_timer = timer_begin(1.0f);

  // What in the unholy f%$ where you doing
  LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
  while (cg_running())
  {
    cg_update();
    const double dt = cg_get_delta_time();

    accumulator += dt;
    while (accumulator >= timeStep)
    {
      // fixed update
      lunaScene_Update();
      accumulator -= timeStep;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      cg_consume_event(&event);
    }
    lunaInput_Update();

    if (lunaInput_IsActionSignalled("+left"))
    {
      lunaObject_Move(player, (vec2){-25.0f * cg_get_delta_time(), 0.0f});
    }
    if (lunaInput_IsActionSignalled("+right"))
    {
      lunaObject_Move(player, (vec2){25.0f * cg_get_delta_time(), 0.0f});
    }
    if (lunaInput_IsActionJustSignalled("+jump"))
    {
      lunaObject_Move(player, (vec2){0.0f, 5.0f});
    }

    vec2 pos = lunaObject_GetPosition(player);
    lunaCamera_SetPosition(&camera, (vec3){pos.x, pos.y, camera.position.z});

    // Profiling code
    totalTime += dt;
    numFrames++;
    if (totalTime >= updateTime)
    {
      curr_showing_fps = ceilf(numFrames / totalTime);
      LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps, numFrames, updateTime,
               (totalTime / (float) (numFrames)));
      numFrames = 0;
      totalTime = 0.0;
    }

    lunaCamera_Update(&camera, rd);

    if (timer_is_done(&cookie_timer))
    {
      cookie_count += floorf(cookies_per_second);
      cookie_timer = timer_begin(1.0f);
    }

    luna_UI_Update();
    if (!bton->was_hovered)
    { // If the mouse is NOT on the button, change it back to it's default color
      bton->color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
    }
    bton->position = v2add((vec2){camera.position.x, camera.position.y}, (vec2){-9.0f, 9.0f});
    bton->size     = (vec2){1.0f, 1.0f};

    if (lunaRenderer_BeginRender(rd))
    {
      lunaScene_Render(rd);

      lunaRenderer_DrawLine(rd, (vec2){camera.position.x, camera.position.y},
                            lunaCamera_GetMouseGlobalPosition(&camera),
                            (vec4){1.0f, 0.0f, 0.0f, 1.0f}, 0);

      lunaRenderer_EndRender(rd);
    }
  }

  lunaScene_Destroy(scn);

  luna_UI_DestroyButton(bton);

  vkDeviceWaitIdle(device);
  ctext_destroy_font(amongus);
  lunaRenderer_Destroy(rd);
  LOG_INFO("done.");
  return 0;
}
