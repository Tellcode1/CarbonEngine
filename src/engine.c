#include "../external/box2d/include/box2d/box2d.h"
#include "../include/containers/cgbitset.h"
#include "../include/containers/cghashmap.h"
#include "../include/containers/cgvector.h"
#include "../include/engine/cengine.h"
#include "../include/engine/lunaCollider.h"
#include "../include/engine/lunaInput.h"
#include "../include/engine/lunaScene.h"
#include "../include/engine/lunaUI.h"
#include "box2d/math_functions.h"

#include <SDL2/SDL.h>

u8     cg_current_frame   = 0;
u64    cg_last_frame_time = 0.0; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
double cg_time            = 0.0;

double cg_delta_time                 = 0.0;
u64    cg_delta_time_last_frame_time = 0;

u64 cg_frame_start       = 0;
u64 cg_fixed_frame_start = 0;
u64 cg_frame_time        = 0;

bool cg_framebuffer_resized = 0;
bool cg_application_running = 1;

static u64 currtime;

// CINPUT VARS
vec2        cinput_mouse_position;
vec2        cinput_last_frame_mouse_position;
cg_bitset_t cinput_kb_state;
cg_bitset_t cinput_last_frame_kb_state;
unsigned    cinput_mouse_state;
unsigned    cinput_last_frame_mouse_state;

void cg_initialize_context(const char* window_title, int window_width, int window_height)
{
  ctx_initialize(window_title, window_width, window_height);

  // This fixes really large values of delta time for the first frame.
  currtime = SDL_GetPerformanceCounter();
}

void cg_consume_event(const SDL_Event* event)
{
  if ((event->type == SDL_QUIT) ||
      ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE)) ||
      (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
    cg_application_running = false;

  if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED ||
                                         event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
    cg_framebuffer_resized = true;
}

void cg_update()
{
  cg_time = (double) SDL_GetTicks64() * (1.0 / 1000.0);

  cg_last_frame_time = currtime;
  currtime           = SDL_GetPerformanceCounter();
  cg_delta_time      = (currtime - cg_last_frame_time) / (double) SDL_GetPerformanceFrequency();
}

// luna
#define LUNA_MAX_OBJECT_COUNT 512

#define VEC2_TO_BVEC2(vec) ((b2Vec2){vec.x, vec.y})
#define BVEC2_TO_VEC2(vec) ((vec2){vec.x, vec.y})

struct lunaCollider
{
  vec2               position;
  vec2               size;
  b2WorldId          world;
  b2BodyId           body;
  b2ShapeId          shape;
  lunaScene*         scene;
  lunaCollider_Shape shape_type;
  lunaCollider_Type  type;
  bool               enabled;
};

struct lunaObject
{
  lunaTransform       transform;
  lunaScene*          scene;
  const char*         name;
  lunaCollider*       col;
  int                 index;
  lunaObjectUpdateFn  update_fn;
  lunaObjectRenderFn  render_fn;
  luna_SpriteRenderer spr_renderer;
};

struct lunaScene
{
  cg_vector_t objects; // the child objects

  const char* scene_name;
  bool        active; // whether the scene is active or not

  b2WorldId world;

  lunaSceneLoadFn   load;
  lunaSceneUnloadFn unload;
};

#define VEC2_TO_BVEC2(vec) ((b2Vec2){vec.x, vec.y})
#define BVEC2_TO_VEC2(vec) ((vec2){vec.x, vec.y})

lunaObject* lunaObject_Create(lunaScene* scene, const char* name, lunaCollider_Type col_type,
                              vec2 position, vec2 size, unsigned flags)
{
  {
    lunaObject stack = {};
    cg_vector_push_back(&scene->objects, &stack);
  }
  lunaObject* obj = cg_vector_get(&scene->objects, scene->objects.m_size - 1);
  obj->name       = name;
  obj->scene      = scene;

  obj->transform.position = position;
  obj->transform.size     = size;
  obj->transform.rotation = (vec4){0.0f, 0.0f, 0.0f, 1.0f};

  obj->spr_renderer                      = (luna_SpriteRenderer){};
  obj->spr_renderer.spr                  = lunaSprite_Empty;
  obj->spr_renderer.tex_coord_multiplier = (vec2){1.0f, 1.0f};
  obj->spr_renderer.color                = (vec4){1.0f, 1.0f, 1.0f, 1.0f};

  obj->index = scene->objects.m_size;

  bool start_enabled = 1;
  if (flags & LUNA_OBJECT_NO_COLLISION)
  {
    start_enabled = 0;
  }
  obj->col =
      lunaCollider_Init(scene, position, size, col_type, LUNA_COLLIDER_SHAPE_RECT, start_enabled);

  return obj;
}

void lunaObject_Destroy(lunaObject* obj)
{
  b2DestroyBody(obj->col->body);
  b2DestroyShape(obj->col->shape, 0);
  cg_vector_remove(&obj->scene->objects, obj->index);
}

void lunaObject_AssignOnUpdateFn(lunaObject* obj, lunaObjectUpdateFn fn)
{
  obj->update_fn = fn;
}

void lunaObject_AssignOnRenderFn(lunaObject* obj, lunaObjectRenderFn fn)
{
  obj->render_fn = fn;
}

void lunaObject_Move(lunaObject* obj, vec2 add)
{
  lunaObject_SetPosition(obj, v2add(lunaObject_GetPosition(obj), add));
}

vec2 lunaObject_GetPosition(const lunaObject* obj)
{
  return obj->transform.position;
}

void lunaObject_SetPosition(lunaObject* obj, vec2 to)
{
  obj->transform.position = to;
  lunaCollider_SetPosition(obj->col, to);
}

vec2 lunaObject_GetSize(const lunaObject* obj)
{
  return obj->transform.size;
}

void lunaObject_SetSize(lunaObject* obj, vec2 to)
{
  obj->transform.size = to;
  lunaCollider_SetSize(obj->col, to);
}

lunaTransform* lunaObject_GetTransform(lunaObject* obj)
{
  return &obj->transform;
}

luna_SpriteRenderer* lunaObject_GetSpriteRenderer(lunaObject* obj)
{
  return &obj->spr_renderer;
}

lunaCollider* lunaObject_GetCollider(lunaObject* obj)
{
  return obj->col;
}

b2BodyId _lunaCollider_BodyInit(lunaScene* scene, lunaCollider_Type type, lunaCollider_Shape shape,
                                vec2 pos, vec2 siz, bool start_enabled)
{
  b2BodyDef body_def = b2DefaultBodyDef();
  if (type == LUNA_COLLIDER_TYPE_STATIC)
  {
    body_def.type = b2_staticBody;
  }
  else if (type == LUNA_COLLIDER_TYPE_DYNAMIC)
  {
    body_def.type = b2_dynamicBody;
  }
  else if (type == LUNA_COLLIDER_TYPE_KINEMATIC)
  {
    body_def.type = b2_kinematicBody;
  }

  if (!start_enabled)
  {
    return (b2BodyId){};
  }

  body_def.position = VEC2_TO_BVEC2(pos);
  b2BodyId body_id  = b2CreateBody(scene->world, &body_def);

  b2ShapeDef shape_def  = b2DefaultShapeDef();
  shape_def.density     = 5.0f;
  shape_def.restitution = 0.0f;

  if (shape == LUNA_COLLIDER_SHAPE_RECT)
  {
    b2Polygon poly = b2MakeBox(siz.x * 0.5f, siz.y * 0.5f);
    b2CreatePolygonShape(body_id, &shape_def, &poly);
  }
  else if (shape == LUNA_COLLIDER_SHAPE_CIRCLE)
  {
    b2Circle circle;
    circle.center = (b2Vec2){0};
    circle.radius = siz.x * 0.5f;
    b2CreateCircleShape(body_id, &shape_def, &circle);
  }
  else if (shape == LUNA_COLLIDER_SHAPE_CAPSULE)
  {
    b2Capsule capsule;
    // capsule.
    cassert(0);
    b2CreateCapsuleShape(body_id, &shape_def, &capsule);
  }

  return body_id;
}

lunaCollider* lunaCollider_Init(lunaScene* scene, vec2 position, vec2 size, lunaCollider_Type type,
                                lunaCollider_Shape shape, bool start_enabled)
{
  lunaCollider* col = calloc(1, sizeof(lunaCollider));

  col->body       = _lunaCollider_BodyInit(scene, type, shape, position, size, start_enabled);
  col->position   = position;
  col->size       = size;
  col->scene      = scene;
  col->shape_type = shape;
  col->type       = type;
  col->world      = scene->world;
  col->enabled    = start_enabled;

  return col;
}

void lunaCollider_Destroy(lunaCollider* col)
{
  if (col && col->enabled)
  {
    b2DestroyBody(col->body);
    free(col);
  }
}

vec2 lunaCollider_GetPosition(const lunaCollider* col)
{
  if (col)
    return col->position;
  else
  {
    return (vec2){};
  }
}

void lunaCollider_SetPosition(lunaCollider* col, vec2 to)
{
  col->position = to;
  if (col->enabled)
  {
    b2Body_SetTransform(col->body, VEC2_TO_BVEC2(to), b2MakeRot(0.0f));
    b2Body_SetAwake(col->body, 1); // wake the bodty
  }
}

vec2 lunaCollider_GetSize(const lunaCollider* col)
{
  return col->size;
}

void lunaCollider_SetSize(lunaCollider* col, vec2 to)
{
  col->size = to;
  if (col->enabled)
  {
    b2DestroyBody(col->body);

    col->body =
        _lunaCollider_BodyInit(col->scene, col->type, col->shape_type, col->position, col->size, 1);
  }
}

typedef struct ray_cast_context
{
  b2ShapeId            raycaster;
  lunaCollider_RayHit* hit;
  bool                 has_hit;
} ray_cast_context;

float cast_result_fn(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
{
  (void) point;
  (void) normal;
  (void) fraction;

  ray_cast_context* ctx = context;
  if (memcmp(&shapeId, &ctx->raycaster, sizeof(b2ShapeId)) == 0)
  {
    return -1.0f;
  }
  ctx->hit->hit              = 1;
  ctx->hit->point_of_contact = BVEC2_TO_VEC2(point);
  return 0.0f;
}

lunaCollider_RayHit lunaCollider_RayCast(const lunaCollider* col, vec2 orig, vec2 dir)
{
  lunaCollider_RayHit hit = {};
  hit.host                = col;

  ray_cast_context ctx = {};
  ctx.raycaster        = col->shape;
  ctx.hit              = &hit;

  b2QueryFilter filter = b2DefaultQueryFilter();
  b2World_CastRay(col->scene->world, VEC2_TO_BVEC2(orig), VEC2_TO_BVEC2(dir), filter,
                  cast_result_fn, &ctx);

  return hit;
}

lunaScene* scene_main = NULL;

lunaScene* lunaScene_Init()
{
  lunaScene* scn = calloc(1, sizeof(lunaScene));

  b2WorldDef world_def = b2DefaultWorldDef();
  world_def.gravity    = (b2Vec2){0.0f, -9.8f};
  scn->world           = b2CreateWorld(&world_def);
  scn->objects         = cg_vector_init(sizeof(lunaObject), 4);

  if (!scene_main)
  {
    lunaScene_ChangeToScene(scn);
  }
  return scn;
}

void lunaScene_Update()
{
  const int   substeps = 4;
  const float timeStep = 1.0 / 60.0;

  b2World_Step(scene_main->world, timeStep, substeps);

  lunaObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < scene_main->objects.m_size; i++)
  {
    lunaCollider* col = objects[i].col;
    if (!col->enabled)
    {
      continue;
    }

    b2Vec2 pos                    = b2Body_GetPosition(col->body);
    objects[i].transform.position = BVEC2_TO_VEC2(pos);
    objects[i].col->position      = BVEC2_TO_VEC2(pos);
  }

  const float dt = cg_get_delta_time();
  for (int i = 0; i < scene_main->objects.m_size; i++)
  {
    if (objects[i].update_fn)
    {
      objects[i].update_fn(dt);
    }
  }
}

void lunaScene_Render(lunaRenderer_t* rd)
{
  const lunaObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < scene_main->objects.m_size; i++)
  {
    vec2                       pos          = lunaObject_GetPosition(&objects[i]);
    vec2                       siz          = lunaObject_GetSize(&objects[i]);
    const luna_SpriteRenderer* spr_renderer = &objects[i].spr_renderer;
    vec2                       texmul       = spr_renderer->tex_coord_multiplier;
    if (spr_renderer->flip_horizontal)
    {
      texmul.x *= -1.0f;
    }
    if (spr_renderer->flip_vertical)
    {
      texmul.y *= -1.0f;
    }
    lunaRenderer_DrawQuad(rd, spr_renderer->spr, texmul, (vec3){pos.x, pos.y, 0.0f},
                          (vec3){siz.x, siz.y, 1.0f}, spr_renderer->color, 0);
  }
  for (int i = 0; i < scene_main->objects.m_size; i++)
  {
    if (objects[i].render_fn)
    {
      objects[i].render_fn(rd);
    }
  }
}
// luna

void lunaScene_Destroy(lunaScene* scene)
{
  b2DestroyWorld(scene->world);
  free(scene);
}

void lunaScene_AssignLoadFn(lunaScene* scene, lunaSceneLoadFn fn)
{
  scene->load = fn;
}

void lunaScene_AssignUnloadFn(lunaScene* scene, lunaSceneUnloadFn fn)
{
  scene->unload = fn;
}

void lunaScene_ChangeToScene(lunaScene* scene)
{
  if (scene_main == scene)
  {
    return;
  }
  if (scene_main && scene_main->unload)
  {
    scene_main->unload(scene);
    lunaScene_Destroy(scene_main);
  }
  if (scene->load)
  {
    scene->load(scene);
  }
  scene_main = scene;
}

// lunaUI
typedef struct luna_UI_Context
{
  cg_vector_t btons;
  cg_vector_t sliders;
  void*       ubmapped;
} luna_UI_Context;

luna_UI_Context luna_ui_ctx;

void luna_UI_Init()
{
  luna_ui_ctx.btons   = cg_vector_init(sizeof(luna_UI_Button), 4);
  luna_ui_ctx.sliders = cg_vector_init(sizeof(luna_UI_Slider), 4);
}

void luna_UI_Shutdown()
{
  cg_vector_destroy(&luna_ui_ctx.btons);
  cg_vector_destroy(&luna_ui_ctx.sliders);
}

luna_UI_Button* luna_UI_CreateButton(lunaSprite_t* spr)
{
  luna_UI_Button bton = (luna_UI_Button){};
  bton.position       = (vec2){};
  bton.size           = (vec2){1.0f, 1.0f};
  bton.color          = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
  bton.spr            = spr;
  cg_vector_push_back(&luna_ui_ctx.btons, &bton);
  return &(
      (luna_UI_Button*) cg_vector_data(&luna_ui_ctx.btons))[cg_vector_size(&luna_ui_ctx.btons) - 1];
}

luna_UI_Slider* luna_UI_CreateSlider()
{
  luna_UI_Slider slider = (luna_UI_Slider){};
  slider.position       = (vec2){};
  slider.size           = (vec2){1.0f, 1.0f};
  slider.min            = 0.0f;
  slider.max            = 1.0f;
  slider.value          = 0.0f;
  slider.bg_color       = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
  slider.slider_color   = (vec4){1.0f, 0.0f, 0.0f, 1.0f};
  slider.bg_sprite      = lunaSprite_Empty;
  slider.slider_sprite  = lunaSprite_Empty;
  slider.interactable   = 0;
  cg_vector_push_back(&luna_ui_ctx.sliders, &slider);
  return (luna_UI_Slider*) cg_vector_get(&luna_ui_ctx.sliders,
                                         cg_vector_size(&luna_ui_ctx.sliders) - 1);
}

void luna_UI_DestroyButton(luna_UI_Button* obj)
{
  lunaSprite_Release(obj->spr);
}

void luna_UI_DestroySlider(luna_UI_Slider* obj)
{
  lunaSprite_Release(obj->slider_sprite);
  lunaSprite_Release(obj->bg_sprite);
}

void luna_UI_Render(lunaRenderer_t* rd)
{
  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++)
  {
    const luna_UI_Button* bton = (luna_UI_Button*) cg_vector_get(&luna_ui_ctx.btons, i);

    lunaRenderer_DrawQuad(rd, bton->spr, (vec2){1.0f, 1.0f},
                          (vec3){bton->position.x, bton->position.y, 0.0f},
                          (vec3){bton->size.x, bton->size.y, 1.0f}, bton->color, 0);
  }

  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.sliders); i++)
  {
    const luna_UI_Slider* slider = (luna_UI_Slider*) cg_vector_get(&luna_ui_ctx.sliders, i);

    if (slider->max == slider->min)
    {
      LOG_ERROR("Slider %i has equal min and max", i);
      continue;
    }

    lunaRenderer_DrawQuad(rd, slider->bg_sprite, (vec2){1.0f, 1.0f},
                          (vec3){slider->position.x, slider->position.y, 0.0f},
                          (vec3){slider->size.x, slider->size.y, 1.0f}, slider->bg_color, 0);

    float pcent = ((slider->value - slider->min) / (slider->max - slider->min));
    pcent       = cmclamp(pcent, 0.0f, 1.0f);

    lunaRenderer_DrawQuad(rd, slider->slider_sprite, (vec2){1.0f, 1.0f},
                          (vec3){slider->position.x + 0.5f * slider->size.x * (pcent - 1.0f),
                                 slider->position.y, 0.0f},
                          (vec3){slider->size.x * pcent, slider->size.y, 1.0f},
                          slider->slider_color, 1);
  }
}

void luna_UI_Update()
{
  const bool mouse_pressed  = lunaInput_IsMouseJustSignalled(SDL_BUTTON_LEFT);
  const vec2 mouse_position = lunaCamera_GetMouseGlobalPosition(&camera);

  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++)
  {
    luna_UI_Button* bton = (luna_UI_Button*) cg_vector_get(&luna_ui_ctx.btons, i);

    const cmrect2d bton_rect = (cmrect2d){.position = bton->position, .size = bton->size};
    if (cmPointInsideRect(&mouse_position, &bton_rect))
    {
      bton->was_hovered = 1;
      if (mouse_pressed && bton->pressed)
      {
        bton->was_pressed = 1;
        bton->pressed(bton);
      }
      else if (bton->hover)
      {
        bton->was_pressed = 0;
        bton->hover(bton);
      }
    }
    else
    {
      bton->was_hovered = 0;
      bton->was_pressed = 0;
    }
  }

  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.sliders); i++)
  {
    luna_UI_Slider* slider = (luna_UI_Slider*) cg_vector_get(&luna_ui_ctx.sliders, i);

    const cmrect2d slider_rect = (cmrect2d){.position = slider->position, .size = slider->size};
    if (slider->interactable && cmPointInsideRect(&mouse_position, &slider_rect))
    {
      if (lunaInput_IsMouseSignalled(LUNA_MOUSE_BUTTON_LEFT))
      {
        float rel_mx     = mouse_position.x - (slider->position.x - slider->size.x * 0.5f);
        float clamped_mx = cmclamp(rel_mx, 0.0f, slider->size.x);
        float percentage = clamped_mx / slider->size.x;
        slider->value    = slider->min + (percentage * (slider->max - slider->min));
        slider->moved    = 1;
      }
      else
      {
        slider->moved = 0;
      }
    }
  }
}
// lunaUI

void LunaEditor_Render(lunaRenderer_t* rd)
{
  (void) rd;
  // lunaRenderer_DrawQuad(rd, sprite_empty, (vec2){1.0f,1.0f},
  // (vec3){-8.0f,0.0f,0.0f}, (vec3){4.0f,20.0f,0.0f},
  // (vec4){1.0f,1.0f,1.0f,1.0f}, 5);
}

vec2        cinput_mouse_position;
vec2        cinput_last_frame_mouse_position;
cg_bitset_t cinput_kb_state;
cg_bitset_t cinput_last_frame_kb_state;
unsigned    cinput_mouse_state;
unsigned    cinput_last_frame_mouse_state;

static cg_hashmap_t* lunaInput_action_mapping;

int lunaInput_SignalAction(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (!ia)
  {
    return -1;
  }
  ia->this_frame = 1;
  if (ia->response)
    ia->response(action, ia);
  return 0;
}

lunaInput_KeyState lunaInput_GetKeyState(const SDL_Scancode sc)
{

  const lunaInput_KeyState this_frame_key_state = cg_bitset_access_bit(&cinput_kb_state, sc);
  const lunaInput_KeyState last_frame_key_state =
      cg_bitset_access_bit(&cinput_last_frame_kb_state, sc);

  // curly braces are beautiful, aren't they?
  if (this_frame_key_state && last_frame_key_state)
  {
    return LUNA_KEY_STATE_HELD;
  }
  else if (!this_frame_key_state && !last_frame_key_state)
  {
    return LUNA_KEY_STATE_NOT_HELD;
  }
  else if (this_frame_key_state && !last_frame_key_state)
  {
    return LUNA_KEY_STATE_PRESSED;
  }
  else if (!this_frame_key_state && last_frame_key_state)
  {
    return LUNA_KEY_STATE_RELEASED;
  }

  return -1;
}

bool lunaInput_IsKeySignalled(const SDL_Scancode sc)
{
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_HELD;
}

bool lunaInput_IsKeyUnsignalled(const SDL_Scancode sc)
{
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_NOT_HELD;
}

bool lunaInput_IsKeyJustSignalled(const SDL_Scancode sc)
{
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_PRESSED;
}

bool lunaInput_IsKeyJustUnsignalled(const SDL_Scancode sc)
{
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_RELEASED;
}

vec2 lunaInput_GetMousePosition(void)
{
  return cinput_mouse_position;
}

vec2 lunaInput_GetLastFrameMousePosition(void)
{
  return cinput_last_frame_mouse_position;
}

vec2 lunaInput_GetMouseDelta(void)
{
  return v2sub(lunaInput_GetLastFrameMousePosition(), lunaInput_GetMousePosition());
}

// button is 1 for left mouse, 2 for middle, 3 for right
bool lunaInput_IsMouseJustSignalled(lunaInput_MouseButton button)
{
  return cinput_mouse_state & SDL_BUTTON(button) &&
         !(cinput_last_frame_mouse_state & SDL_BUTTON(button));
}

bool lunaInput_IsMouseSignalled(lunaInput_MouseButton button)
{
  return cinput_mouse_state & SDL_BUTTON((int) button);
}

bool str_equal(const void* key1, const void* key2, unsigned long keysize)
{
  (void) keysize;
  const char* str1 = key1;
  const char* str2 = key2;
  return strcmp(str1, str2) == 0;
}

void lunaInput_Init()
{
  lunaInput_action_mapping =
      cg_hashmap_init(16, sizeof(const char*), sizeof(lunaInput_Action), NULL, str_equal);

  cinput_kb_state                  = cg_bitset_init(SDL_NUM_SCANCODES);
  cinput_last_frame_kb_state       = cg_bitset_init(SDL_NUM_SCANCODES);
  cinput_mouse_position            = (vec2){};
  cinput_last_frame_mouse_position = (vec2){};
}

void lunaInput_Update()
{
  int mx, my;
  cinput_last_frame_mouse_state = cinput_mouse_state;
  cinput_mouse_state            = SDL_GetMouseState(&mx, &my);

  const float width  = luna_GetWindowSize().width;
  const float height = luna_GetWindowSize().height;

  cinput_last_frame_mouse_position = cinput_mouse_position;
  cinput_mouse_position.x          = ((float) mx / width) * 2.0f - 1.0f;
  cinput_mouse_position.y          = ((float) my / height) * 2.0f - 1.0f;
  cinput_mouse_position.y *= -1.0f;

  const u8* const sdl_kb_state = SDL_GetKeyboardState(NULL);
  cg_bitset_copy_from(&cinput_last_frame_kb_state, &cinput_kb_state);
  for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
  {
    cg_bitset_set_bit_to(&cinput_kb_state, i, sdl_kb_state[i]);
  }

  int        __i = 0;
  ch_node_t* node;

  int mouse_state = SDL_GetMouseState(NULL, NULL);

  while ((node = cg_hashmap_iterate(lunaInput_action_mapping, &__i)) != NULL)
  {
    lunaInput_Action* ia = (lunaInput_Action*) node->value;

    ia->last_frame = ia->this_frame;

    if (ia->key != 0)
    {
      ia->this_frame = sdl_kb_state[ia->key];
      if (ia->response)
        ia->response((const char*) node->key, ia);
    }
    else if (ia->mouse != 255)
    {
      ia->this_frame = (mouse_state & SDL_BUTTON(ia->mouse)) != 0;
      if (ia->response)
        ia->response((const char*) node->key, ia);
    }
    else
    {
      ia->this_frame = 0;
    }
  }
}

void lunaInput_BindFunctionToAction(const char* action, lunaInput_ActionResponseFn response)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return;
  ia->response = response;
}

void lunaInput_BindKeyToAction(SDL_Scancode key, const char* action)
{
  lunaInput_Action ia = {};
  ia.key              = key;
  cg_hashmap_insert(lunaInput_action_mapping, action, &ia);
}

void lunaInput_BindMouseToAction(int bton, const char* action)
{
  lunaInput_Action ia = {};
  ia.mouse            = bton;
  cg_hashmap_insert(lunaInput_action_mapping, action, &ia);
}

void lunaInput_UnbindAction(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return;
  ia->key      = 0;
  ia->mouse    = 255;
  ia->response = NULL;
}

bool lunaInput_IsActionSignalled(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && ia->last_frame;
}

bool lunaInput_IsActionJustSignalled(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && !ia->last_frame;
}

bool lunaInput_IsActionUnsignalled(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && !ia->last_frame;
}

bool lunaInput_IsActionJustUnsignalled(const char* action)
{
  lunaInput_Action* ia = cg_hashmap_find(lunaInput_action_mapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && ia->last_frame;
}