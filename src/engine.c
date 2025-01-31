#include "../include/engine/engine.h"
#include "../common/containers/bitset.h"
#include "../common/containers/dynarray.h"
#include "../common/containers/hashmap.h"
#include "../external/box2d/include/box2d/box2d.h"
#include "../include/engine/UI.h"
#include "../include/engine/camera.h"
#include "../include/engine/collider.h"
#include "../include/engine/input.h"
#include "../include/engine/object.h"
#include "../include/engine/scene.h"

#include "../common/string.h"
#include <SDL2/SDL.h>

u8         NV_CurrentFrame             = 0;
u64        NV_LastFrameTime            = 0; // div by SDL_GetPerformanceCounterFrequency to get actual time.
double     NV_Time                     = 0.0;

double     NV_DeltaTime                = 0.0;

u64        NV_FrameStartTime           = 0;
u64        NV_FixedFrameStartTime      = 0;
u64        NV_FrameTime                = 0;

bool       NV_WindowFramebufferResized = 0;
bool       NV_ApplicationRunning       = 1;

static u64 SDLTime;

// CINPUT VARS
vec2        g_NV_input_MousePosition;
vec2        g_NV_input_LastFrameMousePosition;
NV_bitset_t g_NV_input_KBState;
NV_bitset_t g_NV_input_LastFrameKBState;
unsigned    g_NV_input_MouseState;
unsigned    g_NV_input_LastFrameMouseState;

void
NV_initialize_context(const char* window_title, int window_width, int window_height)
{
  window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  NV_assert(window != NULL);

  _NVVK_initialize_context(window_title, window_width, window_height);

  // This fixes really large values of delta time for the first frame.
  SDLTime = SDL_GetPerformanceCounter();
}

void
NV_consume_event(const SDL_Event* event)
{
  if ((event->type == SDL_QUIT) || ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE))
      || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
    NV_ApplicationRunning = false;

  if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
    NV_WindowFramebufferResized = true;
}

void
NV_update()
{
  NV_Time          = (double)SDL_GetTicks64() * (1.0 / 1000.0);

  NV_LastFrameTime = SDLTime;
  SDLTime          = SDL_GetPerformanceCounter();
  NV_DeltaTime     = (SDLTime - NV_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
}

// NV
#define NOVA_MAX_OBJECT_COUNT 512

#define VEC2_TO_BVEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define BVEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

struct NV_collider_t
{
  b2BodyId         body;
  vec2             position;
  vec2             size;
  b2WorldId        world;
  b2ShapeId        shape;
  NV_scene_t*         scene;
  NV_collider_shape shape_type;
  NV_collider_type  type;
  bool             enabled;
  uint32_t         layer, mask;
};

struct NVObject
{
  NVTransform       transform;
  NV_scene_t*          scene;
  const char*       name;
  NV_collider_t*       col;
  int               index;
  NVObjectUpdateFn  update_fn;
  NVObjectRenderFn  render_fn;
  NV_SpriteRenderer spr_renderer;
};

struct NV_scene_t
{
  NV_dynarray_t   objects; // the child objects

  const char*     scene_name;
  bool            active; // whether the scene is active or not

  b2WorldId       world;

  NV_scene_load_fn   load;
  NV_scene_unload_fn unload;
};

#define VEC2_TO_BVEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define BVEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

NVObject*
NVObject_Create(NV_scene_t* scene, const char* name, NV_collider_type col_type, uint64_t layer, uint64_t mask, vec2 position, vec2 size, unsigned flags)
{
  {
    NVObject stack = {};
    NV_dynarray_push_back(&scene->objects, &stack);
  }
  NVObject* obj                          = NV_dynarray_get(&scene->objects, scene->objects.m_size - 1);
  obj->name                              = name;
  obj->scene                             = scene;

  obj->transform.position                = position;
  obj->transform.size                    = size;
  obj->transform.rotation                = (vec4){ 0.0f, 0.0f, 0.0f, 1.0f };

  obj->spr_renderer                      = (NV_SpriteRenderer){};
  obj->spr_renderer.spr                  = NV_sprite_empty;
  obj->spr_renderer.tex_coord_multiplier = (vec2){ 1.0f, 1.0f };
  obj->spr_renderer.color                = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };

  obj->index                             = scene->objects.m_size;

  bool start_enabled                     = 1;
  if (flags & NOVA_OBJECT_NO_COLLISION)
  {
    start_enabled = 0;
  }
  obj->col = NV_collider_Init(scene, position, size, col_type, NOVA_COLLIDER_SHAPE_RECT, layer, mask, start_enabled);

  return obj;
}

void
NVObject_Destroy(NVObject* obj)
{
  if (!obj)
    return;

  if (obj->col)
  {
    b2DestroyBody(obj->col->body);
    b2DestroyShape(obj->col->shape, 0);
  }
  NV_dynarray_remove(&obj->scene->objects, obj->index);
}

void
NVObject_AssignOnUpdateFn(NVObject* obj, NVObjectUpdateFn fn)
{
  obj->update_fn = fn;
}

void
NVObject_AssignOnRenderFn(NVObject* obj, NVObjectRenderFn fn)
{
  obj->render_fn = fn;
}

void
NVObject_Move(NVObject* obj, vec2 add)
{
  NVObject_SetPosition(obj, v2add(NVObject_GetPosition(obj), add));
}

vec2
NVObject_GetPosition(const NVObject* obj)
{
  return obj->transform.position;
}

void
NVObject_SetPosition(NVObject* obj, vec2 to)
{
  obj->transform.position = to;
  if (obj->col && obj->col->enabled)
  {
    NV_collider_SetPosition(obj->col, to);
  }
}

vec2
NVObject_GetSize(const NVObject* obj)
{
  return obj->transform.size;
}

void
NVObject_SetSize(NVObject* obj, vec2 to)
{
  obj->transform.size = to;
  NV_collider_SetSize(obj->col, to);
}

NVTransform*
NVObject_GetTransform(NVObject* obj)
{
  return &obj->transform;
}

NV_SpriteRenderer*
NVObject_GetSpriteRenderer(NVObject* obj)
{
  return &obj->spr_renderer;
}

NV_collider_t*
NVObject_GetCollider(NVObject* obj)
{
  return obj->col;
}

const char*
NVObject_GetName(NVObject* obj)
{
  return obj->name;
}

void
NVObject_SetName(NVObject* obj, const char* name)
{
  obj->name = name;
}

b2BodyId
_NV_collider_BodyInit(NV_scene_t* scene, NV_collider_type type, NV_collider_shape shape, vec2 pos, vec2 siz, uint64_t layer, uint64_t mask, bool start_enabled)
{
  b2BodyDef body_def = b2DefaultBodyDef();
  if (type == NOVA_COLLIDER_TYPE_STATIC)
  {
    body_def.type = b2_staticBody;
  }
  else if (type == NOVA_COLLIDER_TYPE_DYNAMIC)
  {
    body_def.type = b2_dynamicBody;
  }
  else if (type == NOVA_COLLIDER_TYPE_KINEMATIC)
  {
    body_def.type = b2_kinematicBody;
  }

  if (!start_enabled)
  {
    return (b2BodyId){};
  }

  body_def.position             = VEC2_TO_BVEC2(pos);
  b2BodyId   body_id            = b2CreateBody(scene->world, &body_def);

  b2ShapeDef shape_def          = b2DefaultShapeDef();
  shape_def.density             = 5.0f;
  shape_def.restitution         = 0.0f;
  shape_def.filter.categoryBits = layer;
  shape_def.filter.maskBits     = mask;
  shape_def.filter.groupIndex   = 0;

  if (shape == NOVA_COLLIDER_SHAPE_RECT)
  {
    b2Polygon poly = b2MakeBox(siz.x, siz.y);
    b2CreatePolygonShape(body_id, &shape_def, &poly);
  }
  else if (shape == NOVA_COLLIDER_SHAPE_CIRCLE)
  {
    b2Circle circle;
    circle.center = (b2Vec2){ 0 };
    circle.radius = siz.x;
    b2CreateCircleShape(body_id, &shape_def, &circle);
  }
  else if (shape == NOVA_COLLIDER_SHAPE_CAPSULE)
  {
    b2Capsule capsule;
    // capsule.
    NV_assert(0);
    b2CreateCapsuleShape(body_id, &shape_def, &capsule);
  }

  return body_id;
}

NV_collider_t*
NV_collider_Init(NV_scene_t* scene, vec2 position, vec2 size, NV_collider_type type, NV_collider_shape shape, uint64_t layer, uint64_t mask, bool start_enabled)
{
  NV_collider_t* col = calloc(1, sizeof(NV_collider_t));

  col->body       = _NV_collider_BodyInit(scene, type, shape, position, size, layer, mask, start_enabled);
  col->position   = position;
  col->size       = size;
  col->scene      = scene;
  col->shape_type = shape;
  col->type       = type;
  col->world      = scene->world;
  col->enabled    = start_enabled;
  col->layer      = layer;
  col->mask       = mask;

  return col;
}

void
NV_collider_Destroy(NV_collider_t* col)
{
  if (col && col->enabled)
  {
    b2DestroyBody(col->body);
    NV_free(col);
  }
}

vec2
NV_collider_GetPosition(const NV_collider_t* col)
{
  if (col)
    return col->position;
  else
  {
    return (vec2){};
  }
}

void
NV_collider_SetPosition(NV_collider_t* col, vec2 to)
{
  col->position = to;
  if (col->enabled)
  {
    b2Body_SetTransform(col->body, VEC2_TO_BVEC2(to), b2MakeRot(0.0f));
    b2Body_SetAwake(col->body, 1); // wake the bodty
  }
}

vec2
NV_collider_GetSize(const NV_collider_t* col)
{
  return col->size;
}

void
NV_collider_SetSize(NV_collider_t* col, vec2 to)
{
  col->size = to;
  if (col->enabled)
  {
    b2DestroyBody(col->body);

    col->body = _NV_collider_BodyInit(col->scene, col->type, col->shape_type, col->position, col->size, col->layer, col->mask, 1);
  }
}

typedef struct ray_cast_context
{
  b2ShapeId          raycaster;
  NV_collider_ray_hit* hit;
  bool               has_hit;
} ray_cast_context;

float
cast_result_fn(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
{
  (void)point;
  (void)normal;
  (void)fraction;

  ray_cast_context* ctx = context;
  if (NV_memcmp(&shapeId, &ctx->raycaster, sizeof(b2ShapeId)) == 0)
  {
    return -1.0f;
  }
  ctx->hit->hit              = 1;
  ctx->hit->point_of_contact = BVEC2_TO_VEC2(point);
  return 0.0f;
}

NV_collider_ray_hit
NV_collider_cast_ray(const NV_collider_t* col, vec2 orig, vec2 dir, uint32_t layer, uint32_t mask)
{
  NV_collider_ray_hit hit = {};
  hit.host              = col;

  ray_cast_context ctx  = {};
  ctx.raycaster         = col->shape;
  ctx.hit               = &hit;

  b2QueryFilter filter  = b2DefaultQueryFilter();
  filter.categoryBits   = layer;
  filter.maskBits       = mask;

  b2World_CastRay(col->scene->world, VEC2_TO_BVEC2(orig), VEC2_TO_BVEC2(dir), filter, cast_result_fn, &ctx);

  return hit;
}

NV_scene_t* scene_main = NULL;

NV_scene_t*
NV_scene_Init()
{
  NV_scene_t*   scn       = calloc(1, sizeof(NV_scene_t));

  b2WorldDef world_def = b2DefaultWorldDef();
  world_def.gravity    = (b2Vec2){ 0.0f, -9.8f };
  scn->world           = b2CreateWorld(&world_def);
  scn->objects         = NV_dynarray_init(sizeof(NVObject), 4);

  if (!scene_main)
  {
    NV_scene_change_to_scene(scn);
  }
  return scn;
}

void
NV_scene_update()
{
  const int   substeps = 4;
  const float timeStep = 1.0 / 60.0;

  b2World_Step(scene_main->world, timeStep, substeps);

  NVObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    NV_collider_t* col = objects[i].col;
    if (!col->enabled)
    {
      continue;
    }

    b2Vec2 pos                    = b2Body_GetPosition(col->body);
    objects[i].transform.position = BVEC2_TO_VEC2(pos);
    objects[i].col->position      = BVEC2_TO_VEC2(pos);
  }

  const float dt = NV_get_delta_time();
  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    if (objects[i].update_fn)
    {
      objects[i].update_fn(dt);
    }
  }
}

void
NV_scene_render(NV_renderer_t* rd)
{
  const NVObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    vec2                     pos          = NVObject_GetPosition(&objects[i]);
    vec2                     siz          = NVObject_GetSize(&objects[i]);
    const NV_SpriteRenderer* spr_renderer = &objects[i].spr_renderer;
    vec2                     texmul       = spr_renderer->tex_coord_multiplier;
    if (spr_renderer->flip_horizontal)
    {
      texmul.x *= -1.0f;
    }
    if (spr_renderer->flip_vertical)
    {
      texmul.y *= -1.0f;
    }
    NV_renderer_render_textured_quad(rd, spr_renderer, (vec3){ pos.x, pos.y, 0.0f }, (vec3){ siz.x, siz.y, 0.0f }, 0);
  }
  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    if (objects[i].render_fn)
    {
      objects[i].render_fn(rd);
    }
  }
}
// NV

void
NV_scene_destroy(NV_scene_t* scene)
{
  b2DestroyWorld(scene->world);
  NV_dynarray_destroy(&scene->objects);
  NV_free(scene);
}

void
NV_scene_assign_load_fn(NV_scene_t* scene, NV_scene_load_fn fn)
{
  scene->load = fn;
}

void
NV_scene_assign_unload_fn(NV_scene_t* scene, NV_scene_unload_fn fn)
{
  scene->unload = fn;
}

void
NV_scene_change_to_scene(NV_scene_t* scene)
{
  if (scene_main == scene)
  {
    return;
  }
  if (scene_main && scene_main->unload)
  {
    scene_main->unload(scene);
    NV_scene_destroy(scene_main);
  }
  if (scene->load)
  {
    scene->load(scene);
  }
  scene_main = scene;
}

// NVUI
typedef struct NVUI_Context
{
  NV_dynarray_t btons;
  NV_dynarray_t sliders;
  void*         ubmapped;
  bool          active;
} NVUI_Context;

NVUI_Context NVUI_ctx;

void
NVUI_Init()
{
  NVUI_ctx.active  = 1;
  NVUI_ctx.btons   = NV_dynarray_init(sizeof(NVUI_Button), 4);
  NVUI_ctx.sliders = NV_dynarray_init(sizeof(NVUI_Slider), 4);
}

void
NVUI_Shutdown()
{
  if (!NVUI_ctx.active)
  {
    return;
  }
  NV_dynarray_destroy(&NVUI_ctx.btons);
  NV_dynarray_destroy(&NVUI_ctx.sliders);
  NVUI_ctx.active = 0;
}

NVUI_Button*
NVUI_CreateButton(NV_sprite* spr)
{
  if (!NVUI_ctx.active)
  {
    NV_LOG_ERROR("NVUI not initialized");
    return NULL;
  }
  NVUI_Button bton        = (NVUI_Button){};
  bton.transform.position = (vec2){};
  bton.transform.size     = (vec2){ 0.5f, 0.5f };
  bton.color              = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
  bton.spr                = spr;
  NV_dynarray_push_back(&NVUI_ctx.btons, &bton);
  return &((NVUI_Button*)NV_dynarray_data(&NVUI_ctx.btons))[NV_dynarray_size(&NVUI_ctx.btons) - 1];
}

NVUI_Slider*
NVUI_CreateSlider()
{
  if (!NVUI_ctx.active)
  {
    NV_LOG_ERROR("NVUI not initialized");
    return NULL;
  }
  NVUI_Slider slider        = (NVUI_Slider){};
  slider.transform.position = (vec2){};
  slider.transform.size     = (vec2){ 0.5f, 1.5f };
  slider.min                = 0.0f;
  slider.max                = 1.0f;
  slider.value              = 0.0f;
  slider.bg_color           = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
  slider.slider_color       = (vec4){ 1.0f, 0.0f, 0.0f, 1.0f };
  slider.bg_sprite          = NV_sprite_empty;
  slider.slider_sprite      = NV_sprite_empty;
  slider.interactable       = 0;
  NV_dynarray_push_back(&NVUI_ctx.sliders, &slider);
  return (NVUI_Slider*)NV_dynarray_get(&NVUI_ctx.sliders, NV_dynarray_size(&NVUI_ctx.sliders) - 1);
}

void
NVUI_DestroyButton(NVUI_Button* obj)
{
  NV_sprite_release(obj->spr);
}

void
NVUI_DestroySlider(NVUI_Slider* obj)
{
  NV_sprite_release(obj->slider_sprite);
  NV_sprite_release(obj->bg_sprite);
}

void
NVUI_Render(NV_renderer_t* rd)
{
  if (!NVUI_ctx.active)
  {
    return;
  }
  for (int i = 0; i < (int)NV_dynarray_size(&NVUI_ctx.btons); i++)
  {
    const NVUI_Button* bton = (NVUI_Button*)NV_dynarray_get(&NVUI_ctx.btons, i);

    const NVTransform* t    = &bton->transform;

    NV_renderer_render_quad(rd, bton->spr, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x, t->position.y, 0.0f }, (vec3){ t->size.x, t->size.y, 1.0f }, bton->color, 0);
  }

  for (int i = 0; i < (int)NV_dynarray_size(&NVUI_ctx.sliders); i++)
  {
    const NVUI_Slider* slider = (NVUI_Slider*)NV_dynarray_get(&NVUI_ctx.sliders, i);

    if (slider->max == slider->min)
    {
      NV_LOG_ERROR("Slider %i has equal min and max", i);
      continue;
    }

    const NVTransform* t = &slider->transform;

    NV_renderer_render_quad(
        rd, slider->bg_sprite, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x, t->position.y, 0.0f }, (vec3){ t->size.x, t->size.y, 1.0f }, slider->bg_color, 0);

    float pcent = ((slider->value - slider->min) / (slider->max - slider->min));
    pcent       = NVM_CLAMP(pcent, 0.0f, 1.0f);

    NV_renderer_render_quad(rd, slider->slider_sprite, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x + 0.5f * t->size.x * (pcent - 1.0f), t->position.y, 0.0f },
        (vec3){ t->size.x * pcent, t->size.y, 1.0f }, slider->slider_color, 1);
  }
}

void
NVUI_Update()
{
  const bool mouse_pressed  = NV_input_is_mouse_just_signalled(SDL_BUTTON_LEFT);
  const vec2 mouse_position = NV_camera_get_global_mouse_position(&camera);

  for (int i = 0; i < (int)NV_dynarray_size(&NVUI_ctx.btons); i++)
  {
    NVUI_Button*       bton      = (NVUI_Button*)NV_dynarray_get(&NVUI_ctx.btons, i);
    const NVTransform* t         = &bton->transform;

    const NVM_rect2d     bton_rect = (NVM_rect2d){ .position = t->position, .size = v2muls(t->size, 2.0f) };
    if (NVM_is_point_inside_rect(&mouse_position, &bton_rect))
    {
      bton->was_hovered = 1;
      if (mouse_pressed && bton->on_click)
      {
        bton->was_clicked = 1;
        bton->on_click(bton);
      }
      else if (bton->on_hover)
      {
        bton->was_clicked = 0;
        bton->on_hover(bton);
      }
    }
    else
    {
      bton->was_hovered = 0;
      bton->was_clicked = 0;
    }
  }

  for (int i = 0; i < (int)NV_dynarray_size(&NVUI_ctx.sliders); i++)
  {
    NVUI_Slider*       slider      = (NVUI_Slider*)NV_dynarray_get(&NVUI_ctx.sliders, i);
    const NVTransform* t           = &slider->transform;

    const NVM_rect2d     slider_rect = (NVM_rect2d){ .position = t->position, .size = v2muls(t->size, 2.0f) };
    if (slider->interactable && NVM_is_point_inside_rect(&mouse_position, &slider_rect))
    {
      if (NV_input_is_mouse_signalled(NOVA_MOUSE_BUTTON_LEFT))
      {
        float rel_mx     = mouse_position.x - (t->position.x - t->size.x * 0.5f);
        float clamped_mx = NVM_CLAMP(rel_mx, 0.0f, t->size.x);
        float percentage = clamped_mx / t->size.x;
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
// NVUI

void
LunaEditor_Render(NV_renderer_t* rd)
{
  (void)rd;
  // NV_renderer_render_quad(rd, sprite_empty, (vec2){1.0f,1.0f},
  // (vec3){-8.0f,0.0f,0.0f}, (vec3){4.0f,20.0f,0.0f},
  // (vec4){1.0f,1.0f,1.0f,1.0f}, 5);
}

vec2                 g_NV_input_mouse_position;
vec2                 g_NV_input_last_frame_mouse_position;
NV_bitset_t          g_NV_input_kb_state;
NV_bitset_t          g_NV_input_last_frame_kb_state;
unsigned             g_NV_input_mouse_state;
unsigned             g_NV_input_last_frame_mouse_state;

static NV_hashmap_t* g_NV_input_action_mapping;

int
NV_input_SignalAction(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (!ia)
  {
    return -1;
  }
  ia->this_frame = 1;
  if (ia->response)
    ia->response(action, ia);
  return 0;
}

NV_input_key_state
NV_input_get_key_state(const SDL_Scancode sc)
{
  const NV_input_key_state this_frame_key_state = NV_bitset_access_bit(&g_NV_input_KBState, sc);
  const NV_input_key_state last_frame_key_state = NV_bitset_access_bit(&g_NV_input_LastFrameKBState, sc);

  // curly braces are beautiful, aren't they?
  if (this_frame_key_state && last_frame_key_state)
  {
    return NOVA_KEY_STATE_HELD;
  }
  else if (!this_frame_key_state && !last_frame_key_state)
  {
    return NOVA_KEY_STATE_NOT_HELD;
  }
  else if (this_frame_key_state && !last_frame_key_state)
  {
    return NOVA_KEY_STATE_PRESSED;
  }
  else if (!this_frame_key_state && last_frame_key_state)
  {
    return NOVA_KEY_STATE_RELEASED;
  }

  return -1;
}

bool
NV_input_IsKeySignalled(const SDL_Scancode sc)
{
  return NV_input_get_key_state(sc) == NOVA_KEY_STATE_HELD;
}

bool
NV_input_IsKeyUnsignalled(const SDL_Scancode sc)
{
  return NV_input_get_key_state(sc) == NOVA_KEY_STATE_NOT_HELD;
}

bool
NV_input_IsKeyJustSignalled(const SDL_Scancode sc)
{
  return NV_input_get_key_state(sc) == NOVA_KEY_STATE_PRESSED;
}

bool
NV_input_IsKeyJustUnsignalled(const SDL_Scancode sc)
{
  return NV_input_get_key_state(sc) == NOVA_KEY_STATE_RELEASED;
}

vec2
NV_input_get_mouse_position(void)
{
  return g_NV_input_MousePosition;
}

vec2
NV_input_GetLastFrameMousePosition(void)
{
  return g_NV_input_LastFrameMousePosition;
}

vec2
NV_input_GetMouseDelta(void)
{
  return v2sub(NV_input_GetLastFrameMousePosition(), NV_input_get_mouse_position());
}

// button is 1 for left mouse, 2 for middle, 3 for right
bool
NV_input_is_mouse_just_signalled(NV_input_mouse_button button)
{
  return g_NV_input_MouseState & SDL_BUTTON(button) && !(g_NV_input_LastFrameMouseState & SDL_BUTTON(button));
}

bool
NV_input_is_mouse_signalled(NV_input_mouse_button button)
{
  return g_NV_input_MouseState & SDL_BUTTON((int)button);
}

bool
str_equal(const void* key1, const void* key2, unsigned long keysize)
{
  (void)keysize;
  const char* str1 = key1;
  const char* str2 = key2;
  return NV_strcmp(str1, str2) == 0;
}

void
NV_input_init()
{
  g_NV_input_action_mapping          = NV_hashmap_init(16, sizeof(const char*), sizeof(NV_input_action), NULL, str_equal);

  g_NV_input_kb_state                = NV_bitset_init(SDL_NUM_SCANCODES);
  g_NV_input_last_frame_kb_state       = NV_bitset_init(SDL_NUM_SCANCODES);
  g_NV_input_mouse_position          = (vec2){};
  g_NV_input_last_frame_mouse_position = (vec2){};
}

void
NV_input_shutdown()
{
  NV_hashmap_destroy(g_NV_input_action_mapping);
  NV_bitset_destroy(&g_NV_input_kb_state);
  NV_bitset_destroy(&g_NV_input_last_frame_kb_state);
}

void
NV_input_update()
{
  int mx, my;
  g_NV_input_last_frame_mouse_state    = g_NV_input_mouse_state;
  g_NV_input_mouse_state             = SDL_GetMouseState(&mx, &my);

  const float width                = NV_get_window_size().width;
  const float height               = NV_get_window_size().height;

  g_NV_input_last_frame_mouse_position = g_NV_input_mouse_position;
  g_NV_input_mouse_position.x        = ((float)mx / width) * 2.0f - 1.0f;
  g_NV_input_mouse_position.y        = ((float)my / height) * 2.0f - 1.0f;
  g_NV_input_mouse_position.y *= -1.0f;

  const u8* const sdl_kb_state = SDL_GetKeyboardState(NULL);
  NV_bitset_copy_from(&g_NV_input_last_frame_kb_state, &g_NV_input_kb_state);
  for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
  {
    NV_bitset_set_bit_to(&g_NV_input_kb_state, i, sdl_kb_state[i]);
  }

  int        __i = 0;
  ch_node_t* node;

  int        mouse_state = SDL_GetMouseState(NULL, NULL);

  while ((node = NV_hashmap_iterate(g_NV_input_action_mapping, &__i)) != NULL)
  {
    NV_input_action* ia = (NV_input_action*)node->value;

    ia->last_frame     = ia->this_frame;

    if (ia->key != 0)
    { // key2 is not checked, most ia's won't have one
      ia->this_frame = sdl_kb_state[ia->key] || sdl_kb_state[ia->key2];
      if (ia->response)
        ia->response((const char*)node->key, ia);
    }
    // ia->mouse is checked independently
    if (ia->mouse != 255)
    {
      // it's or'd with ia->this_frame so that we can call the response multiple times, as expected.
      ia->this_frame = ia->this_frame || (mouse_state & SDL_BUTTON(ia->mouse)) != 0;
      if (ia->response)
        ia->response((const char*)node->key, ia);
    }
    else
    {
      ia->this_frame = 0;
    }
  }
}

void
NV_input_bind_function_to_action(const char* action, NV_input_action_response_fn response)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return;
  ia->response = response;
}

void
NV_input_bind_key_to_action(SDL_Scancode key, const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (!ia)
  {
    NV_input_action w = { .key = key };
    NV_hashmap_insert(g_NV_input_action_mapping, action, &w);
  }
  else
  {
    if (ia->key != SDL_SCANCODE_UNKNOWN)
      ia->key2 = key;
    else
    {
      ia->key = key;
    }
  }
}

void
NV_input_bind_mouse_to_action(int bton, const char* action)
{
  NV_input_action ia = {};
  ia.mouse          = bton;
  NV_hashmap_insert(g_NV_input_action_mapping, action, &ia);
}

void
NV_input_unbind_action(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return;
  ia->key      = SDL_SCANCODE_UNKNOWN;
  ia->key2     = SDL_SCANCODE_UNKNOWN;
  ia->mouse    = 255;
  ia->response = NULL;
}

bool
NV_input_is_action_signalled(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && ia->last_frame;
}

bool
NV_input_is_action_just_signalled(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && !ia->last_frame;
}

bool
NV_input_is_action_unsignalled(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && !ia->last_frame;
}

bool
NV_input_is_action_just_unsignalled(const char* action)
{
  NV_input_action* ia = NV_hashmap_find(g_NV_input_action_mapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && ia->last_frame;
}