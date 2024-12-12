#include "../include/lunaObject.h"
#include "../include/defines.h"
#include "../include/lunaGFX.h"
#include "../external/box2d/include/box2d/box2d.h"

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
    LunaObjectEventFn event_fn;
} luna_Object;

#define OBJECT_COUNT_MAX 512
luna_Object objects[ OBJECT_COUNT_MAX ];
int obji = 0;

void luna_Object_Initialize()
{
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, -9.8f};
    world = b2CreateWorld(&world_def);
}

luna_Object *luna_CreateObject(const char *name, bool is_static, vec2 position, vec2 size)
{
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

    luna_Object *obj = &objects[obji];
    *obj = (luna_Object){};
    obj->name = name;
    obj->position = position;
    obj->size = size;
    obj->body = body_id;
    obj->world = world;
    obj->shape = shape_id;
    obj->index = obji;
    // obj->event_fn = event_fn;
    obji++;

    return obj;
}

void luna_ObjectsUpdate()
{
    const float timeStep = 1.0f / 60.0f;
    const int substeps = 4;

    for (int i = 0; i < obji; i++) {
        b2Vec2 vel = b2Body_GetLinearVelocity( objects[i].body );
        objects[i].velocity = (vec2){ vel.x, vel.y };
    }

    b2World_Step(world, timeStep, substeps);

    for (int i = 0; i < obji; i++) {
        // oh my god they stack up so beautifully....
        b2BodyId body = objects[i].body;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Vec2 vel = b2Body_GetLinearVelocity(body);
        objects[i].position = (vec2){ pos.x, pos.y };
        objects[i].velocity = (vec2){ vel.x, vel.y };
    }
}

void luna_ObjectsRender(luna_Renderer_t *rd)
{
    for (int i = 0; i < obji; i++) {
        vec2 pos = objects[i].position;
        vec2 siz = objects[i].size;
        luna_Renderer_DrawQuad(
            rd,
            (vec3){ pos.x, pos.y, 0.0f },
            (vec3){ siz.x, siz.y, 1.0f },
            (vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
        );
    }
}

typedef struct ray_cast_context {
    b2ShapeId raycaster;
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
    ctx->has_hit = 1;
    return 0.0f;
}

bool luna_ObjectRayCast(const luna_Object *obj, vec2 orig, vec2 dir)
{
    ray_cast_context ctx = {};
    ctx.raycaster = obj->shape;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(world, *(b2Vec2 *)&orig, *(b2Vec2 *)&dir, filter, cast_result_fn, &ctx);

    return ctx.has_hit;
}

void luna_ObjectMove(luna_Object *obj, vec2 add)
{
    obj->velocity = v2add(obj->velocity, add);
    b2Body_SetLinearVelocity(obj->body, *(b2Vec2 *)&obj->velocity);
}

vec2 luna_ObjectGetPosition(const luna_Object *obj)
{
    return obj->position;
}

vec2 luna_ObjectGetVelocity(const luna_Object *obj)
{
    b2Vec2 vel = b2Body_GetLinearVelocity(obj->body);
    return (vec2){ vel.x, vel.y };
}

void luna_ObjectSetVelocity(luna_Object *obj, vec2 vel)
{
    obj->velocity = vel;
    b2Body_SetLinearVelocity(obj->body, (b2Vec2){ vel.x, vel.y });
}

void __luna_Object_PassEventToObjects(const SDL_Event *evt)
{
    for (int i = 0; i < obji; i++) {
        if (objects[i].event_fn) {
            objects[i].event_fn(evt);
        }
    }
}
