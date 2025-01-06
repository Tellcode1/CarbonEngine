#include "../include/cengine.h"
#include "../include/cinput.h"
#include "../include/lunaImage.h"
#include "../include/lunaScene.h"
#include "../include/lunaCollider.h"
#include "../include/lunaUI.h"
#include "../include/containers/cgvector.h"
#include "../include/printf.h"

#include "../external/box2d/include/box2d/box2d.h"

#include <SDL2/SDL.h>

u8 cg_current_frame = 0;
u64 cg_last_frame_time = 0.0; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
double cg_time = 0.0;

double cg_delta_time = 0.0;
u64 cg_delta_time_last_frame_time = 0;

u64 cg_frame_start = 0;
u64 cg_fixed_frame_start = 0;
u64 cg_frame_time = 0;

bool cg_framebuffer_resized = 0;
bool cg_application_running = 1;

static u64 currtime;

// CINPUT VARS
vec2 cinput_mouse_position;
vec2 cinput_last_frame_mouse_position;
cg_bitset_t cinput_kb_state;
cg_bitset_t cinput_last_frame_kb_state;
unsigned cinput_mouse_state;
unsigned cinput_last_frame_mouse_state;

void cg_initialize_context(const char *window_title, int window_width, int window_height)
{
    ctx_initialize(window_title, window_width, window_height);

    // This fixes really large values of delta time for the first frame.
    currtime = SDL_GetPerformanceCounter();
}

void cg_consume_event(const SDL_Event *event)
{
    if ((event->type == SDL_QUIT) || ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE)) || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        cg_application_running = false;

    if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        cg_framebuffer_resized = true;
}

void cg_update() {
    cg_time = (double)SDL_GetTicks64() * (1.0 / 1000.0);

    cg_last_frame_time = currtime;
    currtime = SDL_GetPerformanceCounter();
    cg_delta_time = (currtime - cg_last_frame_time) / (double)SDL_GetPerformanceFrequency();
}

// lunaImage
#include <png.h>
#include <jpeglib.h>

const char* get_file_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "piss";
    return dot + 1;
}

lunaImage lunaImage_Load(const char *path)
{
    const char *ext = get_file_extension(path);
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) {
        return lunaImage_LoadJPEG(path);
    } else if (strcmp(ext, "png") == 0) {
        return lunaImage_LoadPNG(path);
    }
    cassert(0);
    return (lunaImage){};
}

uint8_t *lunaImage_PadChannels(const lunaImage *src, int dst_channels) {
    const int src_channels = luna_FormatGetNumChannels(src->fmt);
    cassert(src_channels < dst_channels);

    uint8_t *dst = calloc(src->w * src->h * dst_channels, sizeof(uint8_t));

    // it looked too bad without the extra lines for the curly brackets..
    // i apologise.
    for (int y = 0; y < src->h; y++) {
        for (int x = 0; x < src->w; x++) {
            for (int c = 0; c < dst_channels; c++) {
                if (c < src_channels) {
                    dst[(y * src->w + x) * dst_channels + c] =
                        src->data[(y * src->w + x) * src_channels + c];
                } else {
                    if (c == 3) {  // alpha channel
                        dst[(y * src->w + x) * dst_channels + c] = 255;
                    } else {
                        dst[(y * src->w + x) * dst_channels + c] = 0;
                    }
                }
            }
        }
    }

    return dst;
}

lunaImage lunaImage_LoadPNG(const char *path)
{
    lunaImage texture = {};
    
    FILE *f = fopen(path, "rb");
    cassert(f != NULL);

    png_struct *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    cassert(png != NULL);
    
    png_info *info = png_create_info_struct(png);
    cassert(info != NULL);

    png_init_io(png, f);
    png_read_info(png, info);

    if (setjmp(png_jmpbuf(png))) {
        cassert(0);
    }

    texture.w = png_get_image_width(png, info);
    texture.h = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    // if image has less than 8 bits per pixel, increase it to 8 bpp
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    png_read_update_info(png, info);

    int channels = png_get_channels(png, info);

    switch (channels) {
        case 1: texture.fmt = LUNA_FORMAT_R8; break;
        case 2: texture.fmt = LUNA_FORMAT_RG8; break;
        case 3: texture.fmt = LUNA_FORMAT_RGB8; break;
        case 4: texture.fmt = LUNA_FORMAT_RGBA8; break;
        default:
            LOG_ERROR("unsupported file(png) format: channels = %d", channels);
            cassert(0);
            break;
        break;
    }

    int rowbytes = png_get_rowbytes(png, info);
    texture.data = (unsigned char*)malloc(rowbytes * texture.h * channels);
    cassert(texture.data != NULL);

    u8 **row_pointers = malloc(sizeof(u8 *) * texture.h);
    for (int y = 0; y < texture.h; y++) {
        row_pointers[y] = texture.data + y * texture.w * luna_FormatGetBytesPerPixel(texture.fmt);
    }

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);
    free(row_pointers);

    return texture;
}

lunaImage lunaImage_LoadJPEG(const char *path)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f;
    lunaImage img = {};

    if ((f = fopen(path, "rb")) == NULL) {
        LOG_ERROR("cimageload :: couldn't open file \"%s\" Are you sure that it exists?\n", path);
        return img;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, 1);

    jpeg_start_decompress(&cinfo);

    img.w = cinfo.output_width;
    img.h = cinfo.output_height;
    
    int channels = cinfo.output_components;

    switch (channels) {
        case 1:
            img.fmt = LUNA_FORMAT_R8;
            break;
        case 3:
            img.fmt = LUNA_FORMAT_RGB8;
            channels = 4;
            break;
        default:
            LOG_ERROR("invalid num channels: %d", channels);
            cassert(0);
            break;
        break;
    }

    img.data = (unsigned char*)malloc(img.w * img.h * luna_FormatGetBytesPerPixel(img.fmt));

    unsigned char *bufarr[1];
    for (int i = 0; i < (int)cinfo.output_height; i++) {
        bufarr[0] = img.data + i * img.w * luna_FormatGetBytesPerPixel(img.fmt);
        jpeg_read_scanlines(&cinfo, bufarr, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);

    return img;
}

void lunaImage_WritePNG(const lunaImage *tex, const char *path)
{
    FILE *f = fopen(path, "wb");
    cassert(f != NULL);

    png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    cassert(png != NULL);

    png_infop info = png_create_info_struct(png);
    cassert(info != NULL);

    if (setjmp(png_jmpbuf(png))) {
        cassert(0);
    }

    png_init_io(png, f);

    int coltype = -1;
    const int numc = luna_FormatGetNumChannels(tex->fmt);
    switch (numc) {
        case 1:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case 2:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case 4:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
    }
    cassert(coltype != -1);

    const int bytesperpixel = luna_FormatGetBytesPerPixel(tex->fmt);
    png_set_IHDR(png, info, tex->w, tex->h, bytesperpixel * 8, coltype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    png_bytep *row_pointers = malloc(sizeof(png_byte *) * tex->h);
    for (int y = 0; y < tex->h; y++) {
        row_pointers[y] = tex->data + y * tex->w * bytesperpixel;
    }
    png_write_image(png, row_pointers);
    free(row_pointers);

    png_write_end(png, NULL);

    fclose(f);
    png_destroy_write_struct(&png, &info);
}
// lunaImage

// luna
#define LUNA_MAX_OBJECT_COUNT 512

#define VEC2_TO_B2VEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define B2VEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

typedef struct lunaCollider {
    vec2 position;
    vec2 size;
    b2WorldId world;
    b2BodyId body;
    b2ShapeId shape;
    lunaScene *scene;
    lunaCollider_Shape shape_type;
    lunaCollider_Type type;
    bool enabled;
} lunaCollider;

typedef struct lunaObject {
    lunaScene *scene;
    const char *name;
    lunaCollider *col;
    int index;
    lunaObjectUpdateFn update_fn;
    lunaObjectRenderFn render_fn;
    luna_SpriteRenderer spr_renderer;
} lunaObject;

typedef struct lunaScene {
    cg_vector_t objects; // the child objects

    const char *scene_name;
    bool active; // whether the scene is active or not

    b2WorldId world;

    lunaSceneLoadFn load;
    lunaSceneUnloadFn unload;
} lunaScene;

#define VEC2_TO_B2VEC2(vec) ((b2Vec2){ vec.x, vec.y })
#define B2VEC2_TO_VEC2(vec) ((vec2){ vec.x, vec.y })

lunaObject *lunaObject_Create(lunaScene *scene, const char *name, bool is_static, vec2 position, vec2 size, unsigned flags)
{
    {
        lunaObject stack = {};
        cg_vector_push_back(&scene->objects, &stack);
    }
    lunaObject *obj = cg_vector_get(&scene->objects, scene->objects.m_size - 1);
    obj->name = name;
    obj->scene = scene;

    obj->spr_renderer = (luna_SpriteRenderer){};
    obj->spr_renderer.spr = sprite_empty;
    obj->spr_renderer.tex_coord_multiplier = (vec2){ 1.0f, 1.0f };
    obj->spr_renderer.color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };

    obj->index = scene->objects.m_size;

    bool start_enabled = 1;
    if (flags & LUNA_OBJECT_NO_COLLISION) {
        start_enabled = 0;
    }
    obj->col = lunaCollider_Init(scene, position, size, (is_static) ? LUNA_COLLIDER_TYPE_STATIC : LUNA_COLLIDER_TYPE_DYNAMIC, LUNA_COLLIDER_SHAPE_RECT, start_enabled);

    return obj;
}


void lunaObject_Destroy(lunaObject *obj)
{
    b2DestroyBody(obj->col->body);
    b2DestroyShape(obj->col->shape, 0);
    cg_vector_remove(&obj->scene->objects, obj->index);
}

void lunaObject_AssignOnUpdateFn(lunaObject *obj, lunaObjectUpdateFn fn)
{
    obj->update_fn = fn;
}

void lunaObject_AssignOnRenderFn(lunaObject *obj, lunaObjectRenderFn fn)
{
    obj->render_fn = fn;
}

void lunaObject_Move(lunaObject *obj, vec2 add)
{
    lunaCollider_SetPosition(obj->col, v2add(lunaCollider_GetPosition(obj->col), add));
}

vec2 lunaObject_GetPosition(const lunaObject *obj)
{
    return lunaCollider_GetPosition(obj->col);;
}

void lunaObject_SetPosition(lunaObject *obj, vec2 to)
{
    lunaCollider_SetPosition(obj->col, to);
}

vec2 lunaObject_GetSize(const lunaObject *obj)
{
    return lunaCollider_GetSize(obj->col);
}

void lunaObject_SetSize(lunaObject *obj, vec2 to)
{
    lunaCollider_SetSize(obj->col, to);
}

luna_SpriteRenderer *lunaObject_GetSpriteRenderer(lunaObject *obj)
{
    return &obj->spr_renderer;
}

b2BodyId _lunaCollider_BodyInit(lunaScene *scene, lunaCollider_Type type, lunaCollider_Shape shape, vec2 pos, vec2 siz, bool start_enabled) {
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

    body_def.position = VEC2_TO_B2VEC2(pos);
    b2BodyId body_id = b2CreateBody(scene->world, &body_def);

    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.density = 5.0f;
    shape_def.restitution = 0.0f;

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

lunaCollider *lunaCollider_Init(lunaScene *scene, vec2 position, vec2 size, lunaCollider_Type type, lunaCollider_Shape shape, bool start_enabled) {
    lunaCollider *col = calloc(1, sizeof(lunaCollider));

    col->body = _lunaCollider_BodyInit(scene, type, shape, position, size, start_enabled);
    col->position = position;
    col->size = size;
    col->scene = scene;
    col->shape_type = shape;
    col->type = type;
    col->world = scene->world;
    col->enabled = start_enabled;

    return col;
}

void lunaCollider_Destroy(lunaCollider *col) {
    if (col && col->enabled) {
        b2DestroyBody(col->body);
        free(col);
    }
}

vec2 lunaCollider_GetPosition(const lunaCollider *col)
{
    if (col)
        return col->position;
    else {
        return (vec2){};
    }
}

void lunaCollider_SetPosition(lunaCollider *col, vec2 to)
{
    col->position = to;
    if (col->enabled) {
        b2Body_SetTransform(col->body, VEC2_TO_B2VEC2(to), b2MakeRot(0.0f));
    }
}

vec2 lunaCollider_GetSize(const lunaCollider *col)
{
    return col->size;
}

void lunaCollider_SetSize(lunaCollider *col, vec2 to)
{
    col->size = to;
    if (col->enabled) {
        b2DestroyBody(col->body);

        col->body = _lunaCollider_BodyInit(col->scene, col->type, col->shape_type, col->position, col->size, 1);
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
    if (memcmp(&shapeId, &ctx->raycaster, sizeof(b2ShapeId)) == 0) {
        return -1.0f;
    }
    ctx->hit->hit = 1;
    ctx->hit->point_of_contact = B2VEC2_TO_VEC2(point);
    return 0.0f;
}

lunaCollider_RayHit lunaCollider_RayCast(const lunaCollider *col, vec2 orig, vec2 dir)
{
    lunaCollider_RayHit hit = {};
    hit.host = col;

    ray_cast_context ctx = {};
    ctx.raycaster = col->shape;
    ctx.hit = &hit;

    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(col->scene->world, VEC2_TO_B2VEC2(orig), VEC2_TO_B2VEC2(dir), filter, cast_result_fn, &ctx);

    return hit;
}

lunaScene *scene_main = NULL;

lunaScene *lunaScene_Init()
{
    lunaScene *scn = calloc(1, sizeof(lunaScene));
    
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0};
    scn->world = b2CreateWorld(&world_def);
    scn->objects = cg_vector_init(sizeof(lunaObject), 4);
    return scn;
}

void lunaScene_Update()
{
    const float timeStep = 1.0f / 60.0f;
    const int substeps = 4;

    b2World_Step(scene_main->world, timeStep, substeps);

    const lunaObject *objects = scene_main->objects.m_data;

    for (int i = 0; i < scene_main->objects.m_size; i++) {
        lunaCollider *col = objects[i].col;
        if (!col->enabled) {
            continue;
        }

        b2BodyId body = col->body;
        b2Vec2 pos = b2Body_GetPosition(body);
        col->position = (vec2){ pos.x, pos.y };
    }

    const float dt = cg_get_delta_time();
    for (int i = 0; i < scene_main->objects.m_size; i++) {
        if (objects[i].update_fn) {
            objects[i].update_fn(dt);
        }
    }
}

void lunaScene_Render(luna_Renderer_t *rd)
{
    const lunaObject *objects = scene_main->objects.m_data;

    for (int i = 0; i < scene_main->objects.m_size; i++) {
        vec2 pos = lunaCollider_GetPosition(objects[i].col);
        vec2 siz = lunaCollider_GetSize(objects[i].col);
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
            spr_renderer->color,
            0
        );
    }
    for (int i = 0; i < scene_main->objects.m_size; i++) {
        if (objects[i].render_fn) {
            objects[i].render_fn(rd);
        }
    }
}
// luna

void lunaScene_Destroy(lunaScene *scene)
{
    b2DestroyWorld(scene->world);
    free(scene);
    
}

void lunaScene_AssignLoadFn(lunaScene *scene, lunaSceneLoadFn fn)
{
    scene->load = fn;
}

void lunaScene_AssignUnloadFn(lunaScene *scene, lunaSceneUnloadFn fn)
{
    scene->unload = fn;
}

void lunaScene_ChangeToScene(lunaScene *scene)
{
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
typedef struct luna_UI_Context {
    cg_vector_t btons;
    cg_vector_t sliders;
    void *ubmapped;
} luna_UI_Context;

luna_UI_Context luna_ui_ctx;

void luna_UI_Init() {
    luna_ui_ctx.btons = cg_vector_init(sizeof(luna_UI_Button), 4);
    luna_ui_ctx.sliders = cg_vector_init(sizeof(luna_UI_Slider), 4);
}

void luna_UI_Shutdown() {
    cg_vector_destroy(&luna_ui_ctx.btons);
    cg_vector_destroy(&luna_ui_ctx.sliders);
}

luna_UI_Button *luna_UI_CreateButton(sprite_t *spr) {
    luna_UI_Button bton = (luna_UI_Button){};
    bton.position = (vec2){};
    bton.size = (vec2){1.0f,1.0f};
    bton.color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
    bton.spr = spr;
    cg_vector_push_back(&luna_ui_ctx.btons, &bton);
    return &((luna_UI_Button *)cg_vector_data(&luna_ui_ctx.btons))[ cg_vector_size(&luna_ui_ctx.btons) - 1 ];
}

luna_UI_Slider *luna_UI_CreateSlider() {
    luna_UI_Slider slider = (luna_UI_Slider){};
    slider.position = (vec2){};
    slider.size = (vec2){1.0f,1.0f};
    slider.min = 0.0f;
    slider.max = 1.0f;
    slider.value = 0.0f;
    slider.bg_color = (vec4){1.0f,1.0f,1.0f,1.0f};
    slider.slider_color = (vec4){1.0f,0.0f,0.0f,1.0f};
    slider.bg_sprite = sprite_empty;
    slider.slider_sprite = sprite_empty;
    slider.interactable = 0;
    cg_vector_push_back(&luna_ui_ctx.sliders, &slider);
    return (luna_UI_Slider *)cg_vector_get(&luna_ui_ctx.sliders, cg_vector_size(&luna_ui_ctx.sliders) - 1);
}

void luna_UI_DestroyButton(luna_UI_Button *obj)
{
    sprite_release(obj->spr);
}

void luna_UI_DestroySlider(luna_UI_Slider *obj)
{
    sprite_release(obj->slider_sprite);
    sprite_release(obj->bg_sprite);
}

void luna_UI_Render(luna_Renderer_t *rd)
{
  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++)
  {
    const luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);

    luna_Renderer_DrawQuad(
        rd,
        bton->spr,
        (vec2){1.0f, 1.0f},
        (vec3){bton->position.x, bton->position.y, 0.0f},
        (vec3){bton->size.x, bton->size.y, 1.0f},
        bton->color,
        0);
  }

  for (int i = 0; i < cg_vector_size(&luna_ui_ctx.sliders); i++)
  {
    const luna_UI_Slider *slider = (luna_UI_Slider *)cg_vector_get(&luna_ui_ctx.sliders, i);

    if (slider->max == slider->min)
    {
      LOG_ERROR("Slider %i has equal min and max", i);
      continue;
    }

    luna_Renderer_DrawQuad(
        rd,
        slider->bg_sprite,
        (vec2){1.0f, 1.0f},
        (vec3){slider->position.x, slider->position.y, 0.0f},
        (vec3){slider->size.x, slider->size.y, 1.0f},
        slider->bg_color,
        0);

    float pcent = ((slider->value - slider->min) / (slider->max - slider->min));
    pcent = cmclamp(pcent, 0.0f, 1.0f);

    luna_Renderer_DrawQuad(
        rd,
        slider->slider_sprite,
        (vec2){1.0f, 1.0f},
        (vec3){slider->position.x + 0.5f * slider->size.x * (pcent - 1.0f), slider->position.y, 0.0f},
        (vec3){slider->size.x * pcent, slider->size.y, 1.0f},
        slider->slider_color,
        1);
  }
}

void luna_UI_Update() {
    const bool mouse_pressed = cinput_is_mouse_pressed(SDL_BUTTON_LEFT);
    const vec2 mouse_position = lunaCamera_GetMouseGlobalPosition(&camera);

    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++) {
        luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);
        
        const cmrect2d bton_rect = (cmrect2d) {
            .position = bton->position,
            .size = bton->size
        };
        if (cmPointInsideRect(&mouse_position, &bton_rect)) {
            bton->was_hovered = 1;
            if (mouse_pressed && bton->pressed) {
                bton->was_pressed = 1;
                bton->pressed(bton);
            } else if (bton->hover) {
                bton->was_pressed = 0;
                bton->hover(bton);
            }
        } else {
            bton->was_hovered = 0;
            bton->was_pressed = 0;
        }
    }

    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.sliders); i++) {
        luna_UI_Slider *slider = (luna_UI_Slider *)cg_vector_get(&luna_ui_ctx.sliders, i);
        
        const cmrect2d slider_rect = (cmrect2d) {
            .position = slider->position,
            .size = slider->size
        };
        if (slider->interactable && cmPointInsideRect(&mouse_position, &slider_rect)) {
            if (cinput_is_mouse_held(LUNA_MOUSE_BUTTON_LEFT)) {
                float rel_mx = mouse_position.x - (slider->position.x - slider->size.x * 0.5f);
                float clamped_mx = cmclamp(rel_mx, 0.0f, slider->size.x);
                float percentage = clamped_mx / slider->size.x;
                slider->value = slider->min + (percentage * (slider->max - slider->min));
                slider->moved = 1;
            } else {
                slider->moved = 0;
            }
        }
    }
}
// lunaUI

#include "../include/editor.h"

void LunaEditor_Render(luna_Renderer_t *rd)
{
    
}