#include "../common/containers/bitset.h"
#include "../common/containers/hashmap.h"
#include "../common/containers/vector.h"
#include "../external/box2d/include/box2d/box2d.h"
#include "../include/engine/engine.h"
#include "../include/engine/camera.h"
#include "../include/engine/collider.h"
#include "../include/engine/input.h"
#include "../include/engine/object.h"
#include "../include/engine/scene.h"
#include "../include/engine/UI.h"

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
vec2        g_NVInput_MousePosition;
vec2        g_NVInput_LastFrameMousePosition;
NV_bitset_t g_NVInput_KBState;
NV_bitset_t g_NVInput_LastFrameKBState;
unsigned    g_NVInput_MouseState;
unsigned    g_NVInput_LastFrameMouseState;

void
NV_initialize_context(const char* window_title, int window_width, int window_height)
{
  _NV_initialize_context_internal(window_title, window_width, window_height);

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
  SDLTime            = SDL_GetPerformanceCounter();
  NV_DeltaTime     = (SDLTime - NV_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
}

// NV
#define NOVA_MAX_OBJECT_COUNT 512

#define VEC2_TO_BVEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define BVEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

struct NVCollider
{
  b2BodyId           body;
  vec2               position;
  vec2               size;
  b2WorldId          world;
  b2ShapeId          shape;
  NVScene*         scene;
  NVCollider_Shape shape_type;
  NVCollider_Type  type;
  bool               enabled;
  uint32_t           layer, mask;
};

struct NVObject
{
  NVTransform       transform;
  NVScene*          scene;
  const char*         name;
  NVCollider*       col;
  int                 index;
  NVObjectUpdateFn  update_fn;
  NVObjectRenderFn  render_fn;
  NV_SpriteRenderer spr_renderer;
};

struct NVScene
{
  NV_vector_t       objects; // the child objects

  const char*       scene_name;
  bool              active; // whether the scene is active or not

  b2WorldId         world;

  NVSceneLoadFn   load;
  NVSceneUnloadFn unload;
};

#define VEC2_TO_BVEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define BVEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

NVObject*
NVObject_Create(NVScene* scene, const char* name, NVCollider_Type col_type, uint64_t layer, uint64_t mask, vec2 position, vec2 size, unsigned flags)
{
  {
    NVObject stack = {};
    NV_vector_push_back(&scene->objects, &stack);
  }
  NVObject* obj                        = NV_vector_get(&scene->objects, scene->objects.m_size - 1);
  obj->name                              = name;
  obj->scene                             = scene;

  obj->transform.position                = position;
  obj->transform.size                    = size;
  obj->transform.rotation                = (vec4){ 0.0f, 0.0f, 0.0f, 1.0f };

  obj->spr_renderer                      = (NV_SpriteRenderer){};
  obj->spr_renderer.spr                  = NVSprite_Empty;
  obj->spr_renderer.tex_coord_multiplier = (vec2){ 1.0f, 1.0f };
  obj->spr_renderer.color                = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };

  obj->index                             = scene->objects.m_size;

  bool start_enabled                     = 1;
  if (flags & NOVA_OBJECT_NO_COLLISION)
  {
    start_enabled = 0;
  }
  obj->col = NVCollider_Init(scene, position, size, col_type, NOVA_COLLIDER_SHAPE_RECT, layer, mask, start_enabled);

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
  NV_vector_remove(&obj->scene->objects, obj->index);
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
    NVCollider_SetPosition(obj->col, to);
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
  NVCollider_SetSize(obj->col, to);
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

NVCollider*
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
_NVCollider_BodyInit(NVScene* scene, NVCollider_Type type, NVCollider_Shape shape, vec2 pos, vec2 siz, uint64_t layer, uint64_t mask, bool start_enabled)
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

NVCollider*
NVCollider_Init(NVScene* scene, vec2 position, vec2 size, NVCollider_Type type, NVCollider_Shape shape, uint64_t layer, uint64_t mask, bool start_enabled)
{
  NVCollider* col = calloc(1, sizeof(NVCollider));

  col->body         = _NVCollider_BodyInit(scene, type, shape, position, size, layer, mask, start_enabled);
  col->position     = position;
  col->size         = size;
  col->scene        = scene;
  col->shape_type   = shape;
  col->type         = type;
  col->world        = scene->world;
  col->enabled      = start_enabled;
  col->layer        = layer;
  col->mask         = mask;

  return col;
}

void
NVCollider_Destroy(NVCollider* col)
{
  if (col && col->enabled)
  {
    b2DestroyBody(col->body);
    NV_free(col);
  }
}

vec2
NVCollider_GetPosition(const NVCollider* col)
{
  if (col)
    return col->position;
  else
  {
    return (vec2){};
  }
}

void
NVCollider_SetPosition(NVCollider* col, vec2 to)
{
  col->position = to;
  if (col->enabled)
  {
    b2Body_SetTransform(col->body, VEC2_TO_BVEC2(to), b2MakeRot(0.0f));
    b2Body_SetAwake(col->body, 1); // wake the bodty
  }
}

vec2
NVCollider_GetSize(const NVCollider* col)
{
  return col->size;
}

void
NVCollider_SetSize(NVCollider* col, vec2 to)
{
  col->size = to;
  if (col->enabled)
  {
    b2DestroyBody(col->body);

    col->body = _NVCollider_BodyInit(col->scene, col->type, col->shape_type, col->position, col->size, col->layer, col->mask, 1);
  }
}

typedef struct ray_cast_context
{
  b2ShapeId            raycaster;
  NVCollider_RayHit* hit;
  bool                 has_hit;
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

NVCollider_RayHit
NVCollider_RayCast(const NVCollider* col, vec2 orig, vec2 dir, uint32_t layer, uint32_t mask)
{
  NVCollider_RayHit hit = {};
  hit.host                = col;

  ray_cast_context ctx    = {};
  ctx.raycaster           = col->shape;
  ctx.hit                 = &hit;

  b2QueryFilter filter    = b2DefaultQueryFilter();
  filter.categoryBits     = layer;
  filter.maskBits         = mask;

  b2World_CastRay(col->scene->world, VEC2_TO_BVEC2(orig), VEC2_TO_BVEC2(dir), filter, cast_result_fn, &ctx);

  return hit;
}

NVScene* scene_main = NULL;

NVScene*
NVScene_Init()
{
  NVScene* scn       = calloc(1, sizeof(NVScene));

  b2WorldDef world_def = b2DefaultWorldDef();
  world_def.gravity    = (b2Vec2){ 0.0f, -9.8f };
  scn->world           = b2CreateWorld(&world_def);
  scn->objects         = NV_vector_init(sizeof(NVObject), 4);

  if (!scene_main)
  {
    NVScene_ChangeToScene(scn);
  }
  return scn;
}

void
NVScene_Update()
{
  const int   substeps = 4;
  const float timeStep = 1.0 / 60.0;

  b2World_Step(scene_main->world, timeStep, substeps);

  NVObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    NVCollider* col = objects[i].col;
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
NVScene_Render(NVRenderer_t* rd)
{
  const NVObject* objects = scene_main->objects.m_data;

  for (int i = 0; i < (int)scene_main->objects.m_size; i++)
  {
    vec2                       pos          = NVObject_GetPosition(&objects[i]);
    vec2                       siz          = NVObject_GetSize(&objects[i]);
    const NV_SpriteRenderer* spr_renderer = &objects[i].spr_renderer;
    vec2                       texmul       = spr_renderer->tex_coord_multiplier;
    if (spr_renderer->flip_horizontal)
    {
      texmul.x *= -1.0f;
    }
    if (spr_renderer->flip_vertical)
    {
      texmul.y *= -1.0f;
    }
    NVRenderer_DrawTexturedQuad(rd, spr_renderer, (vec3){ pos.x, pos.y, 0.0f }, (vec3){ siz.x, siz.y, 0.0f }, 0);
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
NVScene_Destroy(NVScene* scene)
{
  b2DestroyWorld(scene->world);
  NV_vector_destroy(&scene->objects);
  NV_free(scene);
}

void
NVScene_AssignLoadFn(NVScene* scene, NVSceneLoadFn fn)
{
  scene->load = fn;
}

void
NVScene_AssignUnloadFn(NVScene* scene, NVSceneUnloadFn fn)
{
  scene->unload = fn;
}

void
NVScene_ChangeToScene(NVScene* scene)
{
  if (scene_main == scene)
  {
    return;
  }
  if (scene_main && scene_main->unload)
  {
    scene_main->unload(scene);
    NVScene_Destroy(scene_main);
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
  NV_vector_t btons;
  NV_vector_t sliders;
  void*       ubmapped;
  bool        active;
} NVUI_Context;

NVUI_Context NVUI_ctx;

void
NVUI_Init()
{
  NVUI_ctx.active = 1;
  NVUI_ctx.btons   = NV_vector_init(sizeof(NVUI_Button), 4);
  NVUI_ctx.sliders = NV_vector_init(sizeof(NVUI_Slider), 4);
}

void
NVUI_Shutdown()
{
  if (!NVUI_ctx.active) {
    return;
  }
  NV_vector_destroy(&NVUI_ctx.btons);
  NV_vector_destroy(&NVUI_ctx.sliders);
  NVUI_ctx.active = 0;
}

NVUI_Button*
NVUI_CreateButton(NVSprite* spr)
{
  if (!NVUI_ctx.active) {
    NV_LOG_ERROR("NVUI not initialized");
    return NULL;
  }
  NVUI_Button bton      = (NVUI_Button){};
  bton.transform.position = (vec2){};
  bton.transform.size     = (vec2){ 0.5f, 0.5f };
  bton.color              = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
  bton.spr                = spr;
  NV_vector_push_back(&NVUI_ctx.btons, &bton);
  return &((NVUI_Button*)NV_vector_data(&NVUI_ctx.btons))[NV_vector_size(&NVUI_ctx.btons) - 1];
}

NVUI_Slider*
NVUI_CreateSlider()
{
  if (!NVUI_ctx.active) {
    NV_LOG_ERROR("NVUI not initialized");
    return NULL;
  }
  NVUI_Slider slider      = (NVUI_Slider){};
  slider.transform.position = (vec2){};
  slider.transform.size     = (vec2){ 0.5f, 1.5f };
  slider.min                = 0.0f;
  slider.max                = 1.0f;
  slider.value              = 0.0f;
  slider.bg_color           = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
  slider.slider_color       = (vec4){ 1.0f, 0.0f, 0.0f, 1.0f };
  slider.bg_sprite          = NVSprite_Empty;
  slider.slider_sprite      = NVSprite_Empty;
  slider.interactable       = 0;
  NV_vector_push_back(&NVUI_ctx.sliders, &slider);
  return (NVUI_Slider*)NV_vector_get(&NVUI_ctx.sliders, NV_vector_size(&NVUI_ctx.sliders) - 1);
}

void
NVUI_DestroyButton(NVUI_Button* obj)
{
  NVSprite_Release(obj->spr);
}

void
NVUI_DestroySlider(NVUI_Slider* obj)
{
  NVSprite_Release(obj->slider_sprite);
  NVSprite_Release(obj->bg_sprite);
}

void
NVUI_Render(NVRenderer_t* rd)
{
  if (!NVUI_ctx.active) {
    return;
  }
  for (int i = 0; i < (int)NV_vector_size(&NVUI_ctx.btons); i++)
  {
    const NVUI_Button* bton = (NVUI_Button*)NV_vector_get(&NVUI_ctx.btons, i);

    const NVTransform* t    = &bton->transform;

    NVRenderer_DrawQuad(rd, bton->spr, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x, t->position.y, 0.0f }, (vec3){ t->size.x, t->size.y, 1.0f }, bton->color, 0);
  }

  for (int i = 0; i < (int)NV_vector_size(&NVUI_ctx.sliders); i++)
  {
    const NVUI_Slider* slider = (NVUI_Slider*)NV_vector_get(&NVUI_ctx.sliders, i);

    if (slider->max == slider->min)
    {
      NV_LOG_ERROR("Slider %i has equal min and max", i);
      continue;
    }

    const NVTransform* t = &slider->transform;

    NVRenderer_DrawQuad(
        rd, slider->bg_sprite, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x, t->position.y, 0.0f }, (vec3){ t->size.x, t->size.y, 1.0f }, slider->bg_color, 0);

    float pcent = ((slider->value - slider->min) / (slider->max - slider->min));
    pcent       = cmclamp(pcent, 0.0f, 1.0f);

    NVRenderer_DrawQuad(rd, slider->slider_sprite, (vec2){ 1.0f, 1.0f }, (vec3){ t->position.x + 0.5f * t->size.x * (pcent - 1.0f), t->position.y, 0.0f },
        (vec3){ t->size.x * pcent, t->size.y, 1.0f }, slider->slider_color, 1);
  }
}

void
NVUI_Update()
{
  const bool mouse_pressed  = NVInput_IsMouseJustSignalled(SDL_BUTTON_LEFT);
  const vec2 mouse_position = NVCamera_GetMouseGlobalPosition(&camera);

  for (int i = 0; i < (int)NV_vector_size(&NVUI_ctx.btons); i++)
  {
    NVUI_Button*       bton      = (NVUI_Button*)NV_vector_get(&NVUI_ctx.btons, i);
    const NVTransform* t         = &bton->transform;

    const cmrect2d       bton_rect = (cmrect2d){ .position = t->position, .size = v2muls(t->size, 2.0f) };
    if (cmPointInsideRect(&mouse_position, &bton_rect))
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

  for (int i = 0; i < (int)NV_vector_size(&NVUI_ctx.sliders); i++)
  {
    NVUI_Slider*       slider      = (NVUI_Slider*)NV_vector_get(&NVUI_ctx.sliders, i);
    const NVTransform* t           = &slider->transform;

    const cmrect2d       slider_rect = (cmrect2d){ .position = t->position, .size = v2muls(t->size, 2.0f) };
    if (slider->interactable && cmPointInsideRect(&mouse_position, &slider_rect))
    {
      if (NVInput_IsMouseSignalled(NOVA_MOUSE_BUTTON_LEFT))
      {
        float rel_mx     = mouse_position.x - (t->position.x - t->size.x * 0.5f);
        float clamped_mx = cmclamp(rel_mx, 0.0f, t->size.x);
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
LunaEditor_Render(NVRenderer_t* rd)
{
  (void)rd;
  // NVRenderer_DrawQuad(rd, sprite_empty, (vec2){1.0f,1.0f},
  // (vec3){-8.0f,0.0f,0.0f}, (vec3){4.0f,20.0f,0.0f},
  // (vec4){1.0f,1.0f,1.0f,1.0f}, 5);
}

vec2                 g_NVInput_MousePosition;
vec2                 g_NVInput_LastFrameMousePosition;
NV_bitset_t          g_NVInput_KBState;
NV_bitset_t          g_NVInput_LastFrameKBState;
unsigned             g_NVInput_MouseState;
unsigned             g_NVInput_LastFrameMouseState;

static NV_hashmap_t* g_NVInput_ActionMapping;

int
NVInput_SignalAction(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (!ia)
  {
    return -1;
  }
  ia->this_frame = 1;
  if (ia->response)
    ia->response(action, ia);
  return 0;
}

NVInput_KeyState
NVInput_GetKeyState(const SDL_Scancode sc)
{
  const NVInput_KeyState this_frame_key_state = NV_bitset_access_bit(&g_NVInput_KBState, sc);
  const NVInput_KeyState last_frame_key_state = NV_bitset_access_bit(&g_NVInput_LastFrameKBState, sc);

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
NVInput_IsKeySignalled(const SDL_Scancode sc)
{
  return NVInput_GetKeyState(sc) == NOVA_KEY_STATE_HELD;
}

bool
NVInput_IsKeyUnsignalled(const SDL_Scancode sc)
{
  return NVInput_GetKeyState(sc) == NOVA_KEY_STATE_NOT_HELD;
}

bool
NVInput_IsKeyJustSignalled(const SDL_Scancode sc)
{
  return NVInput_GetKeyState(sc) == NOVA_KEY_STATE_PRESSED;
}

bool
NVInput_IsKeyJustUnsignalled(const SDL_Scancode sc)
{
  return NVInput_GetKeyState(sc) == NOVA_KEY_STATE_RELEASED;
}

vec2
NVInput_GetMousePosition(void)
{
  return g_NVInput_MousePosition;
}

vec2
NVInput_GetLastFrameMousePosition(void)
{
  return g_NVInput_LastFrameMousePosition;
}

vec2
NVInput_GetMouseDelta(void)
{
  return v2sub(NVInput_GetLastFrameMousePosition(), NVInput_GetMousePosition());
}

// button is 1 for left mouse, 2 for middle, 3 for right
bool
NVInput_IsMouseJustSignalled(NVInput_MouseButton button)
{
  return g_NVInput_MouseState & SDL_BUTTON(button) && !(g_NVInput_LastFrameMouseState & SDL_BUTTON(button));
}

bool
NVInput_IsMouseSignalled(NVInput_MouseButton button)
{
  return g_NVInput_MouseState & SDL_BUTTON((int)button);
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
NVInput_Init()
{
  g_NVInput_ActionMapping          = NV_hashmap_init(16, sizeof(const char*), sizeof(NVInput_Action), NULL, str_equal);

  g_NVInput_KBState                = NV_bitset_init(SDL_NUM_SCANCODES);
  g_NVInput_LastFrameKBState       = NV_bitset_init(SDL_NUM_SCANCODES);
  g_NVInput_MousePosition          = (vec2){};
  g_NVInput_LastFrameMousePosition = (vec2){};
}

void
NVInput_Shutdown()
{
  NV_hashmap_destroy(g_NVInput_ActionMapping);
  NV_bitset_destroy(&g_NVInput_KBState);
  NV_bitset_destroy(&g_NVInput_LastFrameKBState);
}

void
NVInput_Update()
{
  int mx, my;
  g_NVInput_LastFrameMouseState    = g_NVInput_MouseState;
  g_NVInput_MouseState             = SDL_GetMouseState(&mx, &my);

  const float width                  = NV_GetWindowSize().width;
  const float height                 = NV_GetWindowSize().height;

  g_NVInput_LastFrameMousePosition = g_NVInput_MousePosition;
  g_NVInput_MousePosition.x        = ((float)mx / width) * 2.0f - 1.0f;
  g_NVInput_MousePosition.y        = ((float)my / height) * 2.0f - 1.0f;
  g_NVInput_MousePosition.y *= -1.0f;

  const u8* const sdl_kb_state = SDL_GetKeyboardState(NULL);
  NV_bitset_copy_from(&g_NVInput_LastFrameKBState, &g_NVInput_KBState);
  for (u32 i = 0; i < SDL_NUM_SCANCODES; i++)
  {
    NV_bitset_set_bit_to(&g_NVInput_KBState, i, sdl_kb_state[i]);
  }

  int        __i = 0;
  ch_node_t* node;

  int        mouse_state = SDL_GetMouseState(NULL, NULL);

  while ((node = NV_hashmap_iterate(g_NVInput_ActionMapping, &__i)) != NULL)
  {
    NVInput_Action* ia = (NVInput_Action*)node->value;

    ia->last_frame       = ia->this_frame;

    if (ia->key != 0)
    { // key2 is not checked, most ia's won't have one
      ia->this_frame = sdl_kb_state[ia->key] || sdl_kb_state[ia->key2];
      if (ia->response)
        ia->response((const char*)node->key, ia);
    }
    // ia->mouse is checked independently
    if (ia->mouse != 255)
    {
      // it's OR'd with ia->this_frame so that we can call the response multiple times, as expected.
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
NVInput_BindFunctionToAction(const char* action, NVInput_ActionResponseFn response)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return;
  ia->response = response;
}

void
NVInput_BindKeyToAction(SDL_Scancode key, const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (!ia)
  {
    NVInput_Action w = { .key = key };
    NV_hashmap_insert(g_NVInput_ActionMapping, action, &w);
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
NVInput_BindMouseToAction(int bton, const char* action)
{
  NVInput_Action ia = {};
  ia.mouse            = bton;
  NV_hashmap_insert(g_NVInput_ActionMapping, action, &ia);
}

void
NVInput_UnbindAction(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return;
  ia->key      = SDL_SCANCODE_UNKNOWN;
  ia->key2     = SDL_SCANCODE_UNKNOWN;
  ia->mouse    = 255;
  ia->response = NULL;
}

bool
NVInput_IsActionSignalled(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && ia->last_frame;
}

bool
NVInput_IsActionJustSignalled(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && !ia->last_frame;
}

bool
NVInput_IsActionUnsignalled(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && !ia->last_frame;
}

bool
NVInput_IsActionJustUnsignalled(const char* action)
{
  NVInput_Action* ia = NV_hashmap_find(g_NVInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && ia->last_frame;
}