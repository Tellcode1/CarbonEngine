#ifndef __C_TEXT_H__
#define __C_TEXT_H__

#include "object.h"
#include "scene.h"

#include "../../common/math/mat.h"
#include "../../common/math/vec2.h"
#include "../../common/math/vec3.h"

#include "../../common/containers/atlas.h"
#include "../../common/containers/hashmap.h"
#include "../../common/containers/string.h"
#include "../../common/containers/vector.h"

#include "../GPU/buffer.h"
#include "../GPU/texture.h"

NOVA_HEADER_START;

typedef enum ctext_hori_align {
  CTEXT_HORI_ALIGN_LEFT   = 0,
  CTEXT_HORI_ALIGN_CENTER = 1,
  CTEXT_HORI_ALIGN_RIGHT  = 2,
} ctext_hori_align;

typedef enum ctext_vert_align {
  CTEXT_VERT_ALIGN_TOP    = 0,
  CTEXT_VERT_ALIGN_CENTER = 1,
  CTEXT_VERT_ALIGN_BOTTOM = 2,
} ctext_vert_align;

// Since the hash is to be used for individual characters, we can expect
// that there will only be one entry for each character.
static inline unsigned ctext_hash(const void *key, int nbytes) {
  (void)nbytes;
  return *(char *)key;
}

// DEPRECATE THIS YOU FOOL
// IT WAS ONLY MEANT FOR SIMPLE TESTING
static const int CTEXT_MAX_FONT_COUNT = 8;
typedef struct cfont_t cfont_t;

typedef struct ctext_text_render_info {
  mat4 model;
  ctext_hori_align horizontal;
  ctext_vert_align vertical;
  vec4 color;
  vec3 position;
  float scale;        // if scale_for_fit is 1, this is multiplied by the calculated scale.
  vec2 bbox;          // The bounding box that the scale will be determined for. Only when scale_for_fit is 1
  bool scale_for_fit; // calculates the scale needed to fit the text into a box
} ctext_text_render_info;

typedef struct ctext_label ctext_label;

extern ctext_label *ctext_create_label(NVScene *scene, cfont_t *fnt);
extern void ctext_destroy_label(ctext_label *label);

extern NVObject *ctext_label_get_object(const ctext_label *label);
extern void ctext_label_set_text(ctext_label *label, const char *text);
extern void ctext_label_set_horizontal_align(ctext_label *label, ctext_hori_align h_align);
extern void ctext_label_set_vertical_align(ctext_label *label, ctext_vert_align v_align);
extern void ctext_label_set_text_scale(ctext_label *label, float scale);

static inline ctext_text_render_info ctext_init_text_render_info() {
  return (ctext_text_render_info){.model         = m4init(1.0f),
                                  .horizontal    = CTEXT_HORI_ALIGN_CENTER,
                                  .vertical      = CTEXT_VERT_ALIGN_CENTER,
                                  .color         = (vec4){1.0f, 1.0f, 1.0f, 1.0f},
                                  .position      = (vec3){0.0f, 0.0f, 0.0f},
                                  .scale         = 1.0f,
                                  .bbox          = (vec2){},
                                  .scale_for_fit = 0};
}

// Initializes the text renderer for ONLY that renderer
extern void ctext_init(struct NVRenderer_t *rd);
extern void ctext_shutdown(struct NVRenderer_t *rd);

extern void ctext_load_font(NVRenderer_t *rd, const char *font_path, int scale, cfont_t **dst);

extern void ctext_destroy_font(cfont_t *fnt);

extern void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...);

extern void ctext_flush_renders(NVRenderer_t *rd);
extern void __ctext_flush_font(NVRenderer_t *rd, cfont_t *fnt);

// Get the scale needed to fit the string in a box
// The scale is calculated as if both the string and the box were at (0,0)
extern float ctext_get_scale_for_fit(const cfont_t *fnt, const NV_string_t *str, vec2 bbox);

typedef struct ctext_glyph {
  float x0, x1, y0, y1;
  float l, r, b, t;
  float advance;
} ctext_glyph;

typedef struct ctext_drawcall_t ctext_drawcall_t;

/* Internal CFont struct. Do not modify yourselves! */
struct cfont_t {
  float line_height;
  float space_width;

  int font_index;
  NV_GPU_Texture *texture;
  NV_GPU_Memory *texture_mem;
  NV_GPU_Sampler *sampler;

  int allocated_size;
  NV_GPU_Buffer buffer;
  NV_GPU_Memory *buffer_mem;
  void *mapped;

  int index_buffer_offset;
  int index_count;
  bool to_render;
  bool buffer_resized;
  bool rendered_this_frame;

  int chars_drawn;
  NV_vector_t /* ctext_drawcall_t */ drawcalls;
  NV_hashmap_t * /* unicode, ctext_glyph ctext_hasher<unicode>> */ glyph_map;

  struct NVRenderer_t *rd;
};

NOVA_HEADER_END;

#endif