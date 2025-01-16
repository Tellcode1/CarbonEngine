#include "../external/box2d/include/box2d/box2d.h"
#include "../include/containers/cgbitset.h"
#include "../include/containers/cghashmap.h"
#include "../include/containers/cgvector.h"
#include "../include/engine/cengine.h"
#include "../include/engine/lunaCollider.h"
#include "../include/engine/lunaCamera.h"
#include "../include/engine/lunaInput.h"
#include "../include/engine/lunaObject.h"
#include "../include/engine/lunaScene.h"
#include "../include/engine/lunaUI.h"

#include "../common/string.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>

u8 luna_CurrentFrame   = 0;
u64 luna_LastFrameTime = 0; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
double luna_Time       = 0.0;

double luna_DeltaTime       = 0.0;

u64 luna_FrameStartTime      = 0;
u64 luna_FixedFrameStartTime = 0;
u64 luna_FrameTime           = 0;

bool luna_WindowFramebufferResized = 0;
bool luna_ApplicationRunning       = 1;

static u64 SDLTime;

// CINPUT VARS
vec2 g_lunaInput_MousePosition;
vec2 g_lunaInput_LastFrameMousePosition;
cg_bitset_t g_lunaInput_KBState;
cg_bitset_t g_lunaInput_LastFrameKBState;
unsigned g_lunaInput_MouseState;
unsigned g_lunaInput_LastFrameMouseState;

void cg_initialize_context(const char *window_title, int window_width, int window_height) {
  ctx_initialize(window_title, window_width, window_height);

  // This fixes really large values of delta time for the first frame.
  SDLTime = SDL_GetPerformanceCounter();
}

void cg_consume_event(const SDL_Event *event) {
  if ((event->type == SDL_QUIT) || ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE)) ||
      (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
    luna_ApplicationRunning = false;

  if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
    luna_WindowFramebufferResized = true;
}

void cg_update() {
  luna_Time = (double)SDL_GetTicks64() * (1.0 / 1000.0);

  luna_LastFrameTime = SDLTime;
  SDLTime            = SDL_GetPerformanceCounter();
  luna_DeltaTime     = (SDLTime - luna_LastFrameTime) / (double)SDL_GetPerformanceFrequency();
}

// luna
#define LUNA_MAX_OBJECT_COUNT 512

#define VEC2_TO_BVEC2(vec)    ((b2Vec2){vec.x, vec.y})
#define BVEC2_TO_VEC2(vec)    ((vec2){vec.x, vec.y})

struct lunaCollider {
  b2BodyId body;
  vec2 position;
  vec2 size;
  b2WorldId world;
  b2ShapeId shape;
  lunaScene *scene;
  lunaCollider_Shape shape_type;
  lunaCollider_Type type;
  bool enabled;
  uint32_t layer, mask;
};

struct lunaObject {
  lunaTransform transform;
  lunaScene *scene;
  const char *name;
  lunaCollider *col;
  int index;
  lunaObjectUpdateFn update_fn;
  lunaObjectRenderFn render_fn;
  luna_SpriteRenderer spr_renderer;
};

struct lunaScene {
  cg_vector_t objects; // the child objects

  const char *scene_name;
  bool active; // whether the scene is active or not

  b2WorldId world;

  lunaSceneLoadFn load;
  lunaSceneUnloadFn unload;
};

#define VEC2_TO_BVEC2(vec) ((b2Vec2){vec.x, vec.y})
#define BVEC2_TO_VEC2(vec) ((vec2){vec.x, vec.y})

lunaObject *lunaObject_Create(lunaScene *scene, const char *name, lunaCollider_Type col_type, uint64_t layer, uint64_t mask, vec2 position, vec2 size,
                              unsigned flags) {
  {
    lunaObject stack = {};
    cg_vector_push_back(&scene->objects, &stack);
  }
  lunaObject *obj = cg_vector_get(&scene->objects, scene->objects.m_size - 1);
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
  if (flags & LUNA_OBJECT_NO_COLLISION) {
    start_enabled = 0;
  }
  obj->col = lunaCollider_Init(scene, position, size, col_type, LUNA_COLLIDER_SHAPE_RECT, layer, mask, start_enabled);

  return obj;
}

void lunaObject_Destroy(lunaObject *obj) {
  if (obj->col) {
    b2DestroyBody(obj->col->body);
    b2DestroyShape(obj->col->shape, 0);
  }
  cg_vector_remove(&obj->scene->objects, obj->index);
}

void lunaObject_AssignOnUpdateFn(lunaObject *obj, lunaObjectUpdateFn fn) {
  obj->update_fn = fn;
}

void lunaObject_AssignOnRenderFn(lunaObject *obj, lunaObjectRenderFn fn) {
  obj->render_fn = fn;
}

void lunaObject_Move(lunaObject *obj, vec2 add) {
  lunaObject_SetPosition(obj, v2add(lunaObject_GetPosition(obj), add));
}

vec2 lunaObject_GetPosition(const lunaObject *obj) {
  return obj->transform.position;
}

void lunaObject_SetPosition(lunaObject *obj, vec2 to) {
  obj->transform.position = to;
  if (obj->col && obj->col->enabled) {
    lunaCollider_SetPosition(obj->col, to);
  }
}

vec2 lunaObject_GetSize(const lunaObject *obj) {
  return obj->transform.size;
}

void lunaObject_SetSize(lunaObject *obj, vec2 to) {
  obj->transform.size = to;
  lunaCollider_SetSize(obj->col, to);
}

lunaTransform *lunaObject_GetTransform(lunaObject *obj) {
  return &obj->transform;
}

luna_SpriteRenderer *lunaObject_GetSpriteRenderer(lunaObject *obj) {
  return &obj->spr_renderer;
}

lunaCollider *lunaObject_GetCollider(lunaObject *obj) {
  return obj->col;
}

const char *lunaObject_GetName(lunaObject *obj) {
  return obj->name;
}

void lunaObject_SetName(lunaObject *obj, const char *name) {
  obj->name = name;
}

b2BodyId _lunaCollider_BodyInit(lunaScene *scene, lunaCollider_Type type, lunaCollider_Shape shape, vec2 pos, vec2 siz, uint64_t layer, uint64_t mask,
                                bool start_enabled) {
  b2BodyDef body_def = b2DefaultBodyDef();
  if (type == LUNA_COLLIDER_TYPE_STATIC) {
    body_def.type = b2_staticBody;
  } else if (type == LUNA_COLLIDER_TYPE_DYNAMIC) {
    body_def.type = b2_dynamicBody;
  } else if (type == LUNA_COLLIDER_TYPE_KINEMATIC) {
    body_def.type = b2_kinematicBody;
  }

  if (!start_enabled) {
    return (b2BodyId){};
  }

  body_def.position = VEC2_TO_BVEC2(pos);
  b2BodyId body_id  = b2CreateBody(scene->world, &body_def);

  b2ShapeDef shape_def          = b2DefaultShapeDef();
  shape_def.density             = 5.0f;
  shape_def.restitution         = 0.0f;
  shape_def.filter.categoryBits = layer;
  shape_def.filter.maskBits     = mask;
  shape_def.filter.groupIndex   = 0;

  if (shape == LUNA_COLLIDER_SHAPE_RECT) {
    b2Polygon poly = b2MakeBox(siz.x * 0.5f, siz.y * 0.5f);
    b2CreatePolygonShape(body_id, &shape_def, &poly);
  } else if (shape == LUNA_COLLIDER_SHAPE_CIRCLE) {
    b2Circle circle;
    circle.center = (b2Vec2){0};
    circle.radius = siz.x * 0.5f;
    b2CreateCircleShape(body_id, &shape_def, &circle);
  } else if (shape == LUNA_COLLIDER_SHAPE_CAPSULE) {
    b2Capsule capsule;
    // capsule.
    cassert(0);
    b2CreateCapsuleShape(body_id, &shape_def, &capsule);
  }

  return body_id;
}

lunaCollider *lunaCollider_Init(lunaScene *scene, vec2 position, vec2 size, lunaCollider_Type type, lunaCollider_Shape shape, uint64_t layer,
                                uint64_t mask, bool start_enabled) {
  lunaCollider *col = calloc(1, sizeof(lunaCollider));

  col->body       = _lunaCollider_BodyInit(scene, type, shape, position, size, layer, mask, start_enabled);
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

void lunaCollider_Destroy(lunaCollider *col) {
  if (col && col->enabled) {
    b2DestroyBody(col->body);
    luna_free(col);
  }
}

vec2 lunaCollider_GetPosition(const lunaCollider *col) {
  if (col)
    return col->position;
  else {
    return (vec2){};
  }
}

void lunaCollider_SetPosition(lunaCollider *col, vec2 to) {
  col->position = to;
  if (col->enabled) {
    b2Body_SetTransform(col->body, VEC2_TO_BVEC2(to), b2MakeRot(0.0f));
    b2Body_SetAwake(col->body, 1); // wake the bodty
  }
}

vec2 lunaCollider_GetSize(const lunaCollider *col) {
  return col->size;
}

void lunaCollider_SetSize(lunaCollider *col, vec2 to) {
  col->size = to;
  if (col->enabled) {
    b2DestroyBody(col->body);

    col->body = _lunaCollider_BodyInit(col->scene, col->type, col->shape_type, col->position, col->size, col->layer, col->mask, 1);
  }
}

typedef struct ray_cast_context {
  b2ShapeId raycaster;
  lunaCollider_RayHit *hit;
  bool has_hit;
} ray_cast_context;

float cast_result_fn(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void *context) {
  (void)point;
  (void)normal;
  (void)fraction;

  ray_cast_context *ctx = context;
  if (luna_memcmp(&shapeId, &ctx->raycaster, sizeof(b2ShapeId)) == 0) {
    return -1.0f;
  }
  ctx->hit->hit              = 1;
  ctx->hit->point_of_contact = BVEC2_TO_VEC2(point);
  return 0.0f;
}

lunaCollider_RayHit lunaCollider_RayCast(const lunaCollider *col, vec2 orig, vec2 dir, uint32_t layer, uint32_t mask) {
  lunaCollider_RayHit hit = {};
  hit.host                = col;

  ray_cast_context ctx = {};
  ctx.raycaster        = col->shape;
  ctx.hit              = &hit;

  b2QueryFilter filter = b2DefaultQueryFilter();
  filter.categoryBits  = layer;
  filter.maskBits      = mask;

  b2World_CastRay(col->scene->world, VEC2_TO_BVEC2(orig), VEC2_TO_BVEC2(dir), filter, cast_result_fn, &ctx);

  return hit;
}

lunaScene *scene_main = NULL;

lunaScene *lunaScene_Init() {
  lunaScene *scn = calloc(1, sizeof(lunaScene));

  b2WorldDef world_def = b2DefaultWorldDef();
  world_def.gravity    = (b2Vec2){0.0f, -9.8f};
  scn->world           = b2CreateWorld(&world_def);
  scn->objects         = cg_vector_init(sizeof(lunaObject), 4);

  if (!scene_main) {
    lunaScene_ChangeToScene(scn);
  }
  return scn;
}

void lunaScene_Update() {
  const int substeps   = 4;
  const float timeStep = 1.0 / 60.0;

  b2World_Step(scene_main->world, timeStep, substeps);

  lunaObject *objects = scene_main->objects.m_data;

  for (int i = 0; i < scene_main->objects.m_size; i++) {
    lunaCollider *col = objects[i].col;
    if (!col->enabled) {
      continue;
    }

    b2Vec2 pos                    = b2Body_GetPosition(col->body);
    objects[i].transform.position = BVEC2_TO_VEC2(pos);
    objects[i].col->position      = BVEC2_TO_VEC2(pos);
  }

  const float dt = cg_get_delta_time();
  for (int i = 0; i < scene_main->objects.m_size; i++) {
    if (objects[i].update_fn) {
      objects[i].update_fn(dt);
    }
  }
}

void lunaScene_Render(lunaRenderer_t *rd) {
  const lunaObject *objects = scene_main->objects.m_data;

  for (int i = 0; i < scene_main->objects.m_size; i++) {
    vec2 pos                                = lunaObject_GetPosition(&objects[i]);
    vec2 siz                                = lunaObject_GetSize(&objects[i]);
    const luna_SpriteRenderer *spr_renderer = &objects[i].spr_renderer;
    vec2 texmul                             = spr_renderer->tex_coord_multiplier;
    if (spr_renderer->flip_horizontal) {
      texmul.x *= -1.0f;
    }
    if (spr_renderer->flip_vertical) {
      texmul.y *= -1.0f;
    }
    lunaRenderer_DrawTexturedQuad(rd, spr_renderer, (vec3){pos.x, pos.y, 0.0f}, (vec3){siz.x, siz.y, 0.0f}, 0);
  }
  for (int i = 0; i < scene_main->objects.m_size; i++) {
    if (objects[i].render_fn) {
      objects[i].render_fn(rd);
    }
  }
}
// luna

void lunaScene_Destroy(lunaScene *scene) {
  b2DestroyWorld(scene->world);
  luna_free(scene);
}

void lunaScene_AssignLoadFn(lunaScene *scene, lunaSceneLoadFn fn) {
  scene->load = fn;
}

void lunaScene_AssignUnloadFn(lunaScene *scene, lunaSceneUnloadFn fn) {
  scene->unload = fn;
}

void lunaScene_ChangeToScene(lunaScene *scene) {
  if (scene_main == scene) {
    return;
  }
  if (scene_main && scene_main->unload) {
    scene_main->unload(scene);
    lunaScene_Destroy(scene_main);
  }
  if (scene->load) {
    scene->load(scene);
  }
  scene_main = scene;
}

// lunaUI
typedef struct lunaUI_Context {
  cg_vector_t btons;
  cg_vector_t sliders;
  void *ubmapped;
} lunaUI_Context;

lunaUI_Context lunaUI_ctx;

void lunaUI_Init() {
  lunaUI_ctx.btons   = cg_vector_init(sizeof(lunaUI_Button), 4);
  lunaUI_ctx.sliders = cg_vector_init(sizeof(lunaUI_Slider), 4);
}

void lunaUI_Shutdown() {
  cg_vector_destroy(&lunaUI_ctx.btons);
  cg_vector_destroy(&lunaUI_ctx.sliders);
}

lunaUI_Button *lunaUI_CreateButton(lunaSprite *spr) {
  lunaUI_Button bton      = (lunaUI_Button){};
  bton.transform.position = (vec2){};
  bton.transform.size     = (vec2){1.0f, 1.0f};
  bton.color              = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
  bton.spr                = spr;
  cg_vector_push_back(&lunaUI_ctx.btons, &bton);
  return &((lunaUI_Button *)cg_vector_data(&lunaUI_ctx.btons))[cg_vector_size(&lunaUI_ctx.btons) - 1];
}

lunaUI_Slider *lunaUI_CreateSlider() {
  lunaUI_Slider slider      = (lunaUI_Slider){};
  slider.transform.position = (vec2){};
  slider.transform.size     = (vec2){1.0f, 1.0f};
  slider.min                = 0.0f;
  slider.max                = 1.0f;
  slider.value              = 0.0f;
  slider.bg_color           = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
  slider.slider_color       = (vec4){1.0f, 0.0f, 0.0f, 1.0f};
  slider.bg_sprite          = lunaSprite_Empty;
  slider.slider_sprite      = lunaSprite_Empty;
  slider.interactable       = 0;
  cg_vector_push_back(&lunaUI_ctx.sliders, &slider);
  return (lunaUI_Slider *)cg_vector_get(&lunaUI_ctx.sliders, cg_vector_size(&lunaUI_ctx.sliders) - 1);
}

void lunaUI_DestroyButton(lunaUI_Button *obj) {
  lunaSprite_Release(obj->spr);
}

void lunaUI_DestroySlider(lunaUI_Slider *obj) {
  lunaSprite_Release(obj->slider_sprite);
  lunaSprite_Release(obj->bg_sprite);
}

void lunaUI_Render(lunaRenderer_t *rd) {
  for (int i = 0; i < cg_vector_size(&lunaUI_ctx.btons); i++) {
    const lunaUI_Button *bton = (lunaUI_Button *)cg_vector_get(&lunaUI_ctx.btons, i);

    const lunaTransform *t = &bton->transform;

    lunaRenderer_DrawQuad(rd, bton->spr, (vec2){1.0f, 1.0f}, (vec3){t->position.x, t->position.y, 0.0f}, (vec3){t->size.x, t->size.y, 1.0f},
                          bton->color, 0);
  }

  for (int i = 0; i < cg_vector_size(&lunaUI_ctx.sliders); i++) {
    const lunaUI_Slider *slider = (lunaUI_Slider *)cg_vector_get(&lunaUI_ctx.sliders, i);

    if (slider->max == slider->min) {
      LOG_ERROR("Slider %i has equal min and max", i);
      continue;
    }

    const lunaTransform *t = &slider->transform;

    lunaRenderer_DrawQuad(rd, slider->bg_sprite, (vec2){1.0f, 1.0f}, (vec3){t->position.x, t->position.y, 0.0f}, (vec3){t->size.x, t->size.y, 1.0f},
                          slider->bg_color, 0);

    float pcent = ((slider->value - slider->min) / (slider->max - slider->min));
    pcent       = cmclamp(pcent, 0.0f, 1.0f);

    lunaRenderer_DrawQuad(rd, slider->slider_sprite, (vec2){1.0f, 1.0f},
                          (vec3){t->position.x + 0.5f * t->size.x * (pcent - 1.0f), t->position.y, 0.0f}, (vec3){t->size.x * pcent, t->size.y, 1.0f},
                          slider->slider_color, 1);
  }
}

void lunaUI_Update() {
  const bool mouse_pressed  = lunaInput_IsMouseJustSignalled(SDL_BUTTON_LEFT);
  const vec2 mouse_position = lunaCamera_GetMouseGlobalPosition(&camera);

  for (int i = 0; i < cg_vector_size(&lunaUI_ctx.btons); i++) {
    lunaUI_Button *bton    = (lunaUI_Button *)cg_vector_get(&lunaUI_ctx.btons, i);
    const lunaTransform *t = &bton->transform;

    const cmrect2d bton_rect = (cmrect2d){.position = t->position, .size = t->size};
    if (cmPointInsideRect(&mouse_position, &bton_rect)) {
      bton->was_hovered = 1;
      if (mouse_pressed && bton->on_click) {
        bton->was_clicked = 1;
        bton->on_click(bton);
      } else if (bton->on_hover) {
        bton->was_clicked = 0;
        bton->on_hover(bton);
      }
    } else {
      bton->was_hovered = 0;
      bton->was_clicked = 0;
    }
  }

  for (int i = 0; i < cg_vector_size(&lunaUI_ctx.sliders); i++) {
    lunaUI_Slider *slider  = (lunaUI_Slider *)cg_vector_get(&lunaUI_ctx.sliders, i);
    const lunaTransform *t = &slider->transform;

    const cmrect2d slider_rect = (cmrect2d){.position = t->position, .size = t->size};
    if (slider->interactable && cmPointInsideRect(&mouse_position, &slider_rect)) {
      if (lunaInput_IsMouseSignalled(LUNA_MOUSE_BUTTON_LEFT)) {
        float rel_mx     = mouse_position.x - (t->position.x - t->size.x * 0.5f);
        float clamped_mx = cmclamp(rel_mx, 0.0f, t->size.x);
        float percentage = clamped_mx / t->size.x;
        slider->value    = slider->min + (percentage * (slider->max - slider->min));
        slider->moved    = 1;
      } else {
        slider->moved = 0;
      }
    }
  }
}
// lunaUI

void LunaEditor_Render(lunaRenderer_t *rd) {
  (void)rd;
  // lunaRenderer_DrawQuad(rd, sprite_empty, (vec2){1.0f,1.0f},
  // (vec3){-8.0f,0.0f,0.0f}, (vec3){4.0f,20.0f,0.0f},
  // (vec4){1.0f,1.0f,1.0f,1.0f}, 5);
}

vec2 g_lunaInput_MousePosition;
vec2 g_lunaInput_LastFrameMousePosition;
cg_bitset_t g_lunaInput_KBState;
cg_bitset_t g_lunaInput_LastFrameKBState;
unsigned g_lunaInput_MouseState;
unsigned g_lunaInput_LastFrameMouseState;

static cg_hashmap_t *g_lunaInput_ActionMapping;

int lunaInput_SignalAction(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (!ia) {
    return -1;
  }
  ia->this_frame = 1;
  if (ia->response)
    ia->response(action, ia);
  return 0;
}

lunaInput_KeyState lunaInput_GetKeyState(const SDL_Scancode sc) {

  const lunaInput_KeyState this_frame_key_state = cg_bitset_access_bit(&g_lunaInput_KBState, sc);
  const lunaInput_KeyState last_frame_key_state = cg_bitset_access_bit(&g_lunaInput_LastFrameKBState, sc);

  // curly braces are beautiful, aren't they?
  if (this_frame_key_state && last_frame_key_state) {
    return LUNA_KEY_STATE_HELD;
  } else if (!this_frame_key_state && !last_frame_key_state) {
    return LUNA_KEY_STATE_NOT_HELD;
  } else if (this_frame_key_state && !last_frame_key_state) {
    return LUNA_KEY_STATE_PRESSED;
  } else if (!this_frame_key_state && last_frame_key_state) {
    return LUNA_KEY_STATE_RELEASED;
  }

  return -1;
}

bool lunaInput_IsKeySignalled(const SDL_Scancode sc) {
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_HELD;
}

bool lunaInput_IsKeyUnsignalled(const SDL_Scancode sc) {
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_NOT_HELD;
}

bool lunaInput_IsKeyJustSignalled(const SDL_Scancode sc) {
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_PRESSED;
}

bool lunaInput_IsKeyJustUnsignalled(const SDL_Scancode sc) {
  return lunaInput_GetKeyState(sc) == LUNA_KEY_STATE_RELEASED;
}

vec2 lunaInput_GetMousePosition(void) {
  return g_lunaInput_MousePosition;
}

vec2 lunaInput_GetLastFrameMousePosition(void) {
  return g_lunaInput_LastFrameMousePosition;
}

vec2 lunaInput_GetMouseDelta(void) {
  return v2sub(lunaInput_GetLastFrameMousePosition(), lunaInput_GetMousePosition());
}

// button is 1 for left mouse, 2 for middle, 3 for right
bool lunaInput_IsMouseJustSignalled(lunaInput_MouseButton button) {
  return g_lunaInput_MouseState & SDL_BUTTON(button) && !(g_lunaInput_LastFrameMouseState & SDL_BUTTON(button));
}

bool lunaInput_IsMouseSignalled(lunaInput_MouseButton button) {
  return g_lunaInput_MouseState & SDL_BUTTON((int)button);
}

bool str_equal(const void *key1, const void *key2, unsigned long keysize) {
  (void)keysize;
  const char *str1 = key1;
  const char *str2 = key2;
  return luna_strcmp(str1, str2) == 0;
}

void lunaInput_Init() {
  g_lunaInput_ActionMapping = cg_hashmap_init(16, sizeof(const char *), sizeof(lunaInput_Action), NULL, str_equal);

  g_lunaInput_KBState                = cg_bitset_init(SDL_NUM_SCANCODES);
  g_lunaInput_LastFrameKBState       = cg_bitset_init(SDL_NUM_SCANCODES);
  g_lunaInput_MousePosition          = (vec2){};
  g_lunaInput_LastFrameMousePosition = (vec2){};
}

void lunaInput_Shutdown() {
  cg_hashmap_destroy(g_lunaInput_ActionMapping);
  cg_bitset_destroy(&g_lunaInput_KBState);
  cg_bitset_destroy(&g_lunaInput_LastFrameKBState);
}

void lunaInput_Update() {
  int mx, my;
  g_lunaInput_LastFrameMouseState = g_lunaInput_MouseState;
  g_lunaInput_MouseState          = SDL_GetMouseState(&mx, &my);

  const float width  = luna_GetWindowSize().width;
  const float height = luna_GetWindowSize().height;

  g_lunaInput_LastFrameMousePosition = g_lunaInput_MousePosition;
  g_lunaInput_MousePosition.x        = ((float)mx / width) * 2.0f - 1.0f;
  g_lunaInput_MousePosition.y        = ((float)my / height) * 2.0f - 1.0f;
  g_lunaInput_MousePosition.y *= -1.0f;

  const u8 *const sdl_kb_state = SDL_GetKeyboardState(NULL);
  cg_bitset_copy_from(&g_lunaInput_LastFrameKBState, &g_lunaInput_KBState);
  for (u32 i = 0; i < SDL_NUM_SCANCODES; i++) {
    cg_bitset_set_bit_to(&g_lunaInput_KBState, i, sdl_kb_state[i]);
  }

  int __i = 0;
  ch_node_t *node;

  int mouse_state = SDL_GetMouseState(NULL, NULL);

  while ((node = cg_hashmap_iterate(g_lunaInput_ActionMapping, &__i)) != NULL) {
    lunaInput_Action *ia = (lunaInput_Action *)node->value;

    ia->last_frame = ia->this_frame;

    if (ia->key != 0) { // key2 is not checked, most ia's won't have one
      ia->this_frame = sdl_kb_state[ia->key] || sdl_kb_state[ia->key2];
      if (ia->response)
        ia->response((const char *)node->key, ia);
    }
    // ia->mouse is checked independently
    if (ia->mouse != 255) {
      // it's OR'd with ia->this_frame so that we can call the response multiple times, as expected.
      ia->this_frame = ia->this_frame || (mouse_state & SDL_BUTTON(ia->mouse)) != 0;
      if (ia->response)
        ia->response((const char *)node->key, ia);
    } else {
      ia->this_frame = 0;
    }
  }
}

void lunaInput_BindFunctionToAction(const char *action, lunaInput_ActionResponseFn response) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return;
  ia->response = response;
}

void lunaInput_BindKeyToAction(SDL_Scancode key, const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (!ia) {
    lunaInput_Action w = {.key = key};
    cg_hashmap_insert(g_lunaInput_ActionMapping, action, &w);
  } else {
    if (ia->key != SDL_SCANCODE_UNKNOWN)
      ia->key2 = key;
    else {
      ia->key = key;
    }
  }
}

void lunaInput_BindMouseToAction(int bton, const char *action) {
  lunaInput_Action ia = {};
  ia.mouse            = bton;
  cg_hashmap_insert(g_lunaInput_ActionMapping, action, &ia);
}

void lunaInput_UnbindAction(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return;
  ia->key      = SDL_SCANCODE_UNKNOWN;
  ia->key2     = SDL_SCANCODE_UNKNOWN;
  ia->mouse    = 255;
  ia->response = NULL;
}

bool lunaInput_IsActionSignalled(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && ia->last_frame;
}

bool lunaInput_IsActionJustSignalled(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return ia->this_frame && !ia->last_frame;
}

bool lunaInput_IsActionUnsignalled(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && !ia->last_frame;
}

bool lunaInput_IsActionJustUnsignalled(const char *action) {
  lunaInput_Action *ia = cg_hashmap_find(g_lunaInput_ActionMapping, action);
  if (ia == NULL)
    return false;
  return !ia->this_frame && ia->last_frame;
}