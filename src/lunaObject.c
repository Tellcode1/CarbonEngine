#include "../include/lunaObject.h"
#include "../include/defines.h"
#include "../include/lunaGFX.h"
#include "../include/cengine.h"
#include "../external/box2d/include/box2d/box2d.h"
#include "../include/sprite.h"

b2WorldId world;

typedef struct luna_Object {
    const char *name;
    vec2 position;
    vec2 velocity;
    vec2 size;
    b2WorldId world;
    b2BodyId body;
    b2ShapeId shape;
    int index;
    bool collider_disabled;
    LunaObjectUpdateFn update_fn;
    LunaObjectRenderFn render_fn;
    luna_SpriteRenderer spr_renderer;
} luna_Object;

#define OBJECT_COUNT_MAX 512
luna_Object objects[ OBJECT_COUNT_MAX ];
int obji = 0;

#define VEC2_TO_B2VEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define B2VEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

void luna_Object_Initialize()
{
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, 0.0f};
    world = b2CreateWorld(&world_def);
}

luna_Object *luna_CreateObject(const char *name, bool is_static, vec2 position, vec2 size, unsigned flags)
{

    luna_Object *obj = &objects[obji];
    *obj = (luna_Object){};
    obj->name = name;
    obj->position = position;
    obj->size = size;
    obj->index = obji;
    obj->spr_renderer = (luna_SpriteRenderer){};
    obj->spr_renderer.spr = sprite_load_disk("../Assets/i want to die.png");
    obj->spr_renderer.tex_coord_multiplier = (vec2){ 1.0f, 1.0f };

    if (!(flags & LUNA_OBJECT_NO_COLLISION)) {
        b2BodyDef body_def = b2DefaultBodyDef();
        if (is_static) {
            body_def.type = b2_staticBody;
        } else {
            body_def.type = b2_dynamicBody;
        }
        body_def.position = (b2Vec2){ position.x, position.y };
        b2BodyId body_id = b2CreateBody(world, &body_def);

        b2Polygon box = b2MakeBox(size.x * 0.5f, size.y * 0.5f);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = 5.0f;
        shape_def.restitution = 0.0f;

        b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shape_def, &box);

        obj->body = body_id;
        obj->world = world;
        obj->shape = shape_id;

        obj->collider_disabled = 0;
    } else {
        obj->collider_disabled = 1;
    }
    obji++;

    return obj;
}

void luna_ObjectsUpdate()
{
    const float timeStep = 1.0f / 60.0f;
    const int substeps = 4;

    for (int i = 0; i < obji; i++) {
        if (objects[i].collider_disabled) {
            continue;
        }
        b2Vec2 vel = b2Body_GetLinearVelocity( objects[i].body );
        objects[i].velocity = (vec2){ vel.x, vel.y };
    }

    b2World_Step(world, timeStep, substeps);

    for (int i = 0; i < obji; i++) {
        if (objects[i].collider_disabled) {
            continue;
        }
        // oh my god they stack up so beautifully....
        b2BodyId body = objects[i].body;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Vec2 vel = b2Body_GetLinearVelocity(body);
        objects[i].position = (vec2){ pos.x, pos.y };
        objects[i].velocity = (vec2){ vel.x, vel.y };
    }


    const float dt = cg_get_delta_time();
    for (int i = 0; i < obji; i++) {
        if (objects[i].update_fn) {
            objects[i].update_fn(dt);
        }
    }
}

void luna_ObjectsRender(luna_Renderer_t *rd)
{
    for (int i = 0; i < obji; i++) {
        vec2 pos = objects[i].position;
        vec2 siz = objects[i].size;
        const luna_SpriteRenderer *spr_renderer = &objects[i].spr_renderer;
        vec2 texmul = spr_renderer->tex_coord_multiplier;
        if (spr_renderer->flip_horizontal) {
            texmul.x *= -1.0f;
        }
        if (spr_renderer->flip_vertical) {
            texmul.y *= -1.0f;
        }
        luna_Renderer_DrawQuad(
            rd,
            spr_renderer->spr,
            texmul,
            (vec3){ pos.x, pos.y, 0.0f },
            (vec3){ siz.x, siz.y, 1.0f },
            (vec4){ 1.0f, 1.0f, 1.0f, 1.0f },
            0
        );
    }
    for (int i = 0; i < obji; i++) {
        if (objects[i].render_fn) {
            objects[i].render_fn(rd);
        }
    }
}

typedef struct ray_cast_context {
    b2ShapeId raycaster;
    luna_ObjectRayHit *hit;
    bool has_hit;
} ray_cast_context;

float cast_result_fn(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void *context) {
    (void)point;
    (void)normal;
    (void)fraction;

    ray_cast_context *ctx = context;
    if (memcmp(&shapeId, &ctx->raycaster, sizeof(b2ShapeId)) == 0) {
        return -1.0f;
    }
    ctx->hit->hit = 1;
    ctx->hit->point_of_contact = B2VEC2_TO_VEC2(point);
    return 0.0f;
}

luna_ObjectRayHit luna_ObjectRayCast(const luna_Object *obj, vec2 orig, vec2 dir)
{
    luna_ObjectRayHit hit = {};
    hit.host = obj;

    ray_cast_context ctx = {};
    ctx.raycaster = obj->shape;
    ctx.hit = &hit;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(world, VEC2_TO_B2VEC2(orig), VEC2_TO_B2VEC2(dir), filter, cast_result_fn, &ctx);

    return hit;
}

void luna_ObjectAssignOnUpdateFn(luna_Object *obj, LunaObjectUpdateFn fn)
{
    obj->update_fn = fn;
}

void luna_ObjectAssignOnRenderFn(luna_Object *obj, LunaObjectRenderFn fn)
{
    obj->render_fn = fn;
}

void luna_ObjectMove(luna_Object *obj, vec2 add)
{
    obj->velocity = v2add(obj->velocity, add);
    if (!obj->collider_disabled) {
        b2Body_SetLinearVelocity(obj->body, VEC2_TO_B2VEC2(obj->velocity));
    }
}

vec2 luna_ObjectGetPosition(const luna_Object *obj)
{
    return obj->position;
}

void luna_ObjectSetPosition(luna_Object *obj, vec2 to)
{
    obj->position = to;
    if (!obj->collider_disabled) {
        b2Body_SetTransform(obj->body, VEC2_TO_B2VEC2(to), b2MakeRot(0.0f));
    }
}

vec2 luna_ObjectGetSize(const luna_Object *obj)
{
    return obj->size;
}

// WARNING: NOT IMPLEMENTED
void luna_ObjectSetSize(luna_Object *obj, vec2 to)
{
    obj->size = to;
}

vec2 luna_ObjectGetVelocity(const luna_Object *obj)
{
    if (!obj->collider_disabled) {
        b2Vec2 vel = b2Body_GetLinearVelocity(obj->body);
        return (vec2){ vel.x, vel.y };
    } else {
        return obj->velocity;
    }
}

void luna_ObjectSetVelocity(luna_Object *obj, vec2 vel)
{
    obj->velocity = vel;
    if (!obj->collider_disabled) {
        b2Body_SetLinearVelocity(obj->body, (b2Vec2){ vel.x, vel.y });
    }
}

luna_SpriteRenderer *luna_ObjectGetSpriteRenderer(luna_Object *obj)
{
    return &obj->spr_renderer;
}