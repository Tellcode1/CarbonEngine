#include "../include/ctext.h"
#include "../include/lunaGFX.h"

#include "../include/cshadermanager.h"
#include "../include/lunaPipeline.h"
#include "../include/lunaVK.h"
#include "../include/lunaImage.h"

#include "../include/cgfxdef.h"

#include "../include/cgfxpipeline.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "../include/ctext_font_file.h"

#define MAX(a,b) ( (a > b) ? a : b )

typedef struct cglyph_vertex_t {
    vec3 pos;
    vec2 uv;
} cglyph_vertex_t;

typedef struct ctext_text_drawcall_t {
    u32 vertex_count;
    u32 index_count;
    u32 index_offset;
    cglyph_vertex_t *vertices;
    u32 *indices;
    vec4 color;
} ctext_text_drawcall_t;

struct push_constants {
    mat4 model;
    vec4 color;
    vec4 outline_color;
};

typedef enum ctext_err_t {
    CTEXT_ERR_SUCCESS,
    CTEXT_ERR_INVALID_GLYPH, // May also mean that there just isn't a glyph
    CTEXT_ERR_WRONG_CACHE, // This cache is not the one you're looking for
    CTEXT_ERR_FILE_ERROR,
} ctext_err_t;

void ctext_load_font_from_ft(const char *font_path, bool sdf, cfont_t *dstref) {
    FT_Library lib; FT_Face face;
    if (FT_Init_FreeType(&lib))
    {
        LOG_ERROR("failed to initialize ft\n");
    }

    if (FT_New_Face(lib, font_path, 0, &face))
    {
        LOG_ERROR("failed to load font file: %s\n", font_path);
        FT_Done_FreeType(lib);
    }
    FT_Set_Pixel_Sizes(face, 0, 256);

    strncpy(dstref->family_name, face->family_name, 127);
    strncpy(dstref->style_name, face->style_name, 127);

    if (face->size->metrics.height) {
        dstref->line_height = -face->size->metrics.height / (f32)face->units_per_EM;
    } else {
        dstref->line_height = -(f32)(face->ascender - face->descender) / face->size->metrics.y_scale;
    }
    dstref->atlas = catlas_init(4096, 4096);

    for (int i = 0; i < 256; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, i);
        if (glyph_index == 0) {
            continue;
        }
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
            continue;
        }

        if (i == ' ') {
            dstref->space_width = (f32)face->glyph->metrics.horiAdvance / (f32)face->units_per_EM;
            continue;
        }

        enum FT_Render_Mode_ render_mode = sdf ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL;
        FT_Render_Glyph(face->glyph, render_mode);
        FT_GlyphSlot g = face->glyph;

        const int w = g->bitmap.width;
        const int h = g->bitmap.rows;
        const unsigned char *buffer = g->bitmap.buffer;

        int x,y;
        if (catlas_add_image(&dstref->atlas, w, h, buffer, &x, &y)) {
            printf("atlas error\n");
            continue;
        }

        FT_Glyph gl;
        FT_Get_Glyph(face->glyph, &gl);

        FT_BBox box;
        FT_Glyph_Get_CBox(gl, FT_GLYPH_BBOX_UNSCALED, &box);

        FT_Done_Glyph(gl);

        const ctext_glyph retglyph = {
            .x0 = box.xMin / (f32)face->units_per_EM,
            .x1 = box.xMax / (f32)face->units_per_EM,
            .y0 = box.yMin / (f32)face->units_per_EM,
            .y1 = box.yMax / (f32)face->units_per_EM,

            .l = (float)(x) / dstref->atlas.width,
            .r = (float)(x + w) / dstref->atlas.width,
            .b = (float)(y + h) / dstref->atlas.height,
            .t = (float)(y) / dstref->atlas.height,

            .advance = (f32)face->glyph->metrics.horiAdvance / (f32)face->units_per_EM
        };

        const char glyphi = i;
        cg_hashmap_insert(dstref->glyph_map, &glyphi, &retglyph);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);
}

/* I have no idea what any of this is */

void ctext_load_font(luna_Renderer_t *rd, const char *font_path, int scale, cfont_t **dst)
{
	if (!rd || !dst) {
		LOG_ERROR("pInfo or dst is NULL!");
	}

    if (scale <= 0.0f) {
        LOG_ERROR("attempting to load a font with 0 fontscale.");
    }

    *dst = calloc(1, sizeof(cfont_t));

    cfont_t *dstref = *dst;

    dstref->glyph_map = cg_hashmap_init(16, sizeof(char), sizeof(ctext_glyph), NULL, NULL);
    dstref->drawcalls = cg_vector_init(sizeof(ctext_text_drawcall_t), 4);

    // we somehow need multiple font cache support
    if (ctext_ff_read("atlas.ff", dstref) != CTEXT_ERR_SUCCESS) {
        ctext_load_font_from_ft(font_path, 0, dstref);
        ctext_ff_write("atlas.ff", dstref);
    }

    luna_GPU_TextureCreateInfo image_info = {
        .format = VK_FORMAT_R8_UNORM,
        .samples = CG_SAMPLE_COUNT_1_SAMPLES,
        .type = VK_IMAGE_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .extent = (VkExtent3D){ .width = dstref->atlas.width, .height = dstref->atlas.height, .depth = 1 },
        .arraylayers = 1,
        .miplevels = 1
    };
    luna_GPU_CreateTexture(&image_info, &dstref->texture);

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, dstref->texture.image, &imageMemoryRequirements);

    luna_GPU_AllocateMemory(imageMemoryRequirements.size, LUNA_GPU_MEMORY_USAGE_GPU_LOCAL, &dstref->texture_mem);
    luna_GPU_BindImageToMemory(&dstref->texture_mem, 0, &dstref->texture);

    const int atlas_w = dstref->atlas.width, atlas_h = dstref->atlas.height;
    luna_VK_StageImageTransfer(
        dstref->texture.image,
        dstref->atlas.data,
        atlas_w,
        atlas_h,
        atlas_w * atlas_h * vk_fmt_get_bpp(VK_FORMAT_R8_UNORM)
    );

    const luna_GPU_SamplerCreateInfo sampler_info = {
        .filter = VK_FILTER_LINEAR,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT
    };
    luna_GPU_CreateSampler(&sampler_info, &dstref->sampler);

    luna_GPU_TextureViewCreateInfo view_info = {
        .format = VK_FORMAT_R8_UNORM,
        .view_type = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    luna_GPU_CreateTextureView(&view_info, &dstref->texture);

    const VkDescriptorImageInfo ctext_bitmap_image_info = {
        .sampler = dstref->sampler.vksampler,
        .imageView = dstref->texture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = rd->ctext->desc_set.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &ctext_bitmap_image_info
    };
    for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++)
    {
        writeSet.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, NULL);
    }
}

void ctext__font_resize_buffer(cfont_t *fnt, int new_buffer_size)
{
    int new_allocation_size;

    if (fnt->allocated_size < new_buffer_size) {
        new_allocation_size = MAX(fnt->allocated_size * 2, new_buffer_size);
    }
    else if (new_buffer_size < (fnt->allocated_size / 3)) {
        new_allocation_size = MAX(fnt->allocated_size / 3, new_buffer_size);
    }
    else
        return;

    vkDeviceWaitIdle(device);
    
    if (fnt->buffer.vkbuffer) {
        luna_GPU_DestroyBuffer(&fnt->buffer);
        luna_GPU_FreeMemory(&fnt->buffer_mem);
    }

    luna_GPU_AllocateMemory(new_allocation_size, LUNA_GPU_MEMORY_USAGE_GPU_LOCAL | LUNA_GPU_MEMORY_USAGE_CPU_WRITEABLE, &fnt->buffer_mem);

    luna_GPU_CreateBuffer(
        new_allocation_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &fnt->buffer
    );
    luna_GPU_BindBufferToMemory(&fnt->buffer_mem, 0, &fnt->buffer);

    fnt->allocated_size = new_allocation_size;
}

void ctext__render_drawcalls(luna_Renderer_t *rd, ccamera *camera, cfont_t *fnt, const mat4 *model) {
    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(rd);
    const VkDeviceSize offsets[] = { 0 };

    struct push_constants pc = {};

    pc.model = m4mul(camera->ortho, m4mul(cam_get_view(camera), *model));

    const VkPipeline pipeline = rd->ctext->pipeline.pipeline;
    const VkPipelineLayout pipeline_layout = rd->ctext->pipeline.pipeline_layout;

    // Viewport && scissor are set by renderer so no need to set them here
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &rd->ctext->desc_set.set, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer.vkbuffer, offsets);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindIndexBuffer(cmd, fnt->buffer.vkbuffer, fnt->index_buffer_offset, VK_INDEX_TYPE_UINT32);

    int offset = 0;
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);

        pc.color = drawcall->color;
        vkCmdPushConstants(
            cmd,
            pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(struct push_constants),
            &pc
        );

        vkCmdDrawIndexed(cmd, drawcall->index_count, 1, 0, offset, 0);
        offset += drawcall->vertex_count;
    }
}

void ctext_end_render(luna_Renderer_t *rd, ccamera *camera, cfont_t *fnt, mat4 model)
{
    if (fnt->to_render) {
        ctext__render_drawcalls(rd, camera, fnt, &model);
    }

    u32 total_vertex_byte_size = 0;
    u32 total_index_count = 0;

    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        const ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        total_vertex_byte_size += drawcall->vertex_count * sizeof(cglyph_vertex_t);
        total_index_count += drawcall->index_count;
    }

    const u32 total_index_byte_size = total_index_count * sizeof(u32);
    const u32 total_buffer_size = total_index_byte_size + total_vertex_byte_size;

    ctext__font_resize_buffer(fnt, total_buffer_size);

    uint8_t *mapped;
    vkMapMemory(device, fnt->buffer_mem.memory, 0, fnt->allocated_size, 0, (void **)&mapped);

    // this may be dumb but I am too

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        const ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        memcpy(
            mapped + vertex_copy_iterator,
            drawcall->vertices,
            drawcall->vertex_count * sizeof(cglyph_vertex_t)
        );
        memcpy(
            mapped + total_vertex_byte_size + index_copy_iterator,
            drawcall->indices,
            drawcall->index_count * sizeof(u32)
        );
        vertex_copy_iterator += drawcall->vertex_count * sizeof(cglyph_vertex_t);
        index_copy_iterator += drawcall->index_count * sizeof(u32);
    }
    vkUnmapMemory(device, fnt->buffer_mem.memory);

    fnt->index_buffer_offset = total_vertex_byte_size;
    fnt->index_count = total_index_count;
    fnt->to_render = true;

    fnt->chars_drawn = 0;
    
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        free(drawcall->vertices);
    }
    cg_vector_clear(&fnt->drawcalls);
}

static cg_vector_t split_string(const cg_string_t *str) {
    cg_vector_t result;
    cg_string_t *substr = NULL;
    const char *str_data = cg_string_data(str);
    const int str_len = cg_string_length(str);
    int i_start = 0;

    result = cg_vector_init(sizeof(cg_string_t *), 16);

    for (int i = 0; i < str_len; i++) {
        if (str_data[i] == '\n') {
            if (i > i_start) {
                substr = cg_string_substring(str, i_start, i - i_start);
                // substr should now handle errors internally
                cg_vector_push_back(&result, &substr);
            }
            i_start = i + 1;
        }
    }

    if (i_start < str_len) {
        substr = cg_string_substring(str, i_start, str_len - i_start);
        cg_vector_push_back(&result, &substr);
    }

    return result;
}

// Get the unscaled size of the string
// Warning: slow
static void ctext_get_text_size(const cfont_t *fnt, const cg_string_t *str, float *w, float *h) {
    const int len = cg_string_length(str);
    float width = 0.0f, height = fnt->line_height;

    for (int i = 0; i < len; i++) {
        const char c = str->data[i];
        switch (c) {
            case ' ':
                width += fnt->space_width;
                continue;
            case '\t':
                width += fnt->space_width * 4.0f;
                continue;
            case '\n':
                height += fnt->line_height;
                continue;
            default:
                const ctext_glyph *glyph = (ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &c);
                if (!glyph) {
                    continue;
                }
                width += glyph->advance;
                continue;
        }
    }
    if (w) { *w = width;  }
    if (h) { *h = -height; }
}

void ctext_begin_render(cfont_t *fnt)
{
    
}

static int render_line(
    const cfont_t *fnt,
    const cg_string_t *str,
    const ctext_text_drawcall_t* drawcall,
    const ctext_text_render_info *pInfo,
    const int glyph_iter,
    f32 x,
    const f32 y,
    float scale
) {
    const int len = cg_string_length(str);

    int iter = 0;
    for (int i = 0; i < len; i++) {
        const char c = str->data[i];
        switch (c) {
            case ' ':
                x += fnt->space_width;
                continue;
            case '\t':
                x += fnt->space_width * 4.0f;
                continue;
            default:
                const ctext_glyph *glyph = (const ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &c);
                if (!glyph) {
                    LOG_INFO("no glyph when rendering char %c", c);
                    continue;
                }
                const f32 glyph_x0 = glyph->x0 + x;
                const f32 glyph_x1 = glyph->x1 + x;
                const f32 glyph_y0 = glyph->y0 + y;
                const f32 glyph_y1 = glyph->y1 + y;

                const int index_offset = (fnt->chars_drawn + iter) * 4;
                cglyph_vertex_t *v_out = drawcall->vertices + (glyph_iter + iter) * 4;
                v_out[0] = (cglyph_vertex_t){ v3muls((vec3){glyph_x0, glyph_y0, pInfo->position.z}, scale), (vec2){glyph->l, glyph->b}, };
                v_out[1] = (cglyph_vertex_t){ v3muls((vec3){glyph_x1, glyph_y0, pInfo->position.z}, scale), (vec2){glyph->r, glyph->b}, };
                v_out[2] = (cglyph_vertex_t){ v3muls((vec3){glyph_x1, glyph_y1, pInfo->position.z}, scale), (vec2){glyph->r, glyph->t}, };
                v_out[3] = (cglyph_vertex_t){ v3muls((vec3){glyph_x0, glyph_y1, pInfo->position.z}, scale), (vec2){glyph->l, glyph->t}  };

                u32 *i_out = drawcall->indices + (glyph_iter + iter) * 6;
                i_out[0] = index_offset;
                i_out[1] = index_offset + 1;
                i_out[2] = index_offset + 2;
                i_out[3] = index_offset + 2;
                i_out[4] = index_offset + 3;
                i_out[5] = index_offset;

                x += glyph->advance;
                iter++;
                continue;
        }
    }

    return iter;
}

static inline int get_effective_length(const char *buf, int buflen) {
    int len = 0;
    for (int i = 0; i < buflen; i++) {
        const char c = buf[i];
        if (c != ' ' && c != '\t' && c != '\n') {
            len++;
        }
    }
    return len;
}

void gen_vertices(
    cfont_t *fnt,
    ctext_text_drawcall_t* drawcall,
    const ctext_text_render_info *pInfo,
    const cg_string_t *str,
    const float scale
)
{
    if (cg_string_length(str) == 0) {
        return;
    }

    cg_vector_t splitted_strings = split_string(str);
    const int old_chars_drawn = fnt->chars_drawn, line_count = splitted_strings.m_size;

    // this is here because the functions above this are supposed to
    // make sure line count is not 0
    cassert(line_count > 0);

    float text_w, text_h;
    ctext_get_text_size(fnt, str, &text_w, &text_h);

    float ypos = pInfo->position.y;

    switch(pInfo->vertical)
    {
        case CTEXT_VERT_ALIGN_CENTER:
            ypos -= text_h / 2.0f;
            break;
        case CTEXT_VERT_ALIGN_BOTTOM:
            ypos -= text_h;
            break;
        case CTEXT_VERT_ALIGN_TOP:
            break;
        default:
            __builtin_unreachable();
            LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)", pInfo->vertical);
            break;
        break;
    }

    for (int i = 0; i < line_count; i++) {;
        // render_line returns the number of chars DRAWN. not the number of characters in the string.
        const cg_string_t *line = ((cg_string_t **)cg_vector_data(&splitted_strings))[i];

        ctext_get_text_size(fnt, line, &text_w, &text_h);

        float xpos = pInfo->position.x;
        switch(pInfo->horizontal) {
            case CTEXT_HORI_ALIGN_CENTER:
                xpos -= text_w / 2.0f;
                break;
            case CTEXT_HORI_ALIGN_RIGHT:
                xpos -= text_w;
                break;
            case CTEXT_HORI_ALIGN_LEFT:
                break;
            default:
                __builtin_unreachable();
                LOG_ERROR("Invalid horizontal alignment. Specified (int)%u. (Implement?)", pInfo->horizontal);
                break;
        }

        fnt->chars_drawn += render_line(
            fnt,
            line,
            drawcall,
            pInfo,
            // This gives us the number of characters this function call has drawn.
            // only this call.
            MAX(fnt->chars_drawn - old_chars_drawn, 0),
            xpos,
            pInfo->position.y + (i * fnt->line_height),
            scale
        );
    }

    for (int i = 0; i < line_count; i++) {
        cg_string_t *str = ((cg_string_t **)cg_vector_data(&splitted_strings))[i];
        cg_string_destroy(str);
    }
    cg_vector_destroy(&splitted_strings);
}

void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...)
{
    cg_string_t *str;
    int num, effective_length;
    {
        char buffer[ 100000 ];
        va_list arg;

        va_start(arg, fmt);
        num = vsnprintf(buffer, 100000, fmt, arg);
        va_end(arg);

        effective_length = get_effective_length(buffer, num);
        if (effective_length == 0) {
            return;
        }

        str = cg_string_init_str(buffer);
    }

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;
    const int allocation_size = (vertex_count * sizeof(cglyph_vertex_t)) + (index_count * sizeof(u32));

    void *allocation = malloc(allocation_size);

    ctext_text_drawcall_t drawcall = {};
    drawcall.vertices = (cglyph_vertex_t *)allocation;
    drawcall.index_offset = (vertex_count * sizeof(cglyph_vertex_t));
    drawcall.indices = (u32 *)((uchar *)allocation + drawcall.index_offset);

    drawcall.color = pInfo->color;

    float scale = pInfo->scale ;
    if (pInfo->scale_for_fit) {
        scale *= ctext_get_scale_for_fit(fnt, str, pInfo->bbox);
    }
    gen_vertices(fnt, &drawcall, pInfo, str, scale);

    drawcall.vertex_count = effective_length * 4;
    drawcall.index_count = effective_length * 6;

    cg_vector_push_back(&fnt->drawcalls, &drawcall);

    cg_string_destroy(str);
}

void ctext_init(struct luna_Renderer_t *rd)
{
    rd->ctext = calloc(1, sizeof(cg_ctext_module));
    cg_ctext_module *ctext = rd->ctext;

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        (VkDescriptorSetLayoutBinding){ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CTEXT_MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };

    luna_DescriptorPool_Init(&ctext->desc_pool);
    luna_AllocateDescriptorSet(&ctext->desc_pool, bindings, array_len(bindings), &ctext->desc_set);

    u32 width, height;
    VkFormat dummyImageFmt;

    // No need to store image memory because it won't ever be deleted (probably)
    // ! you should probably store it and free it.
    VkDeviceMemory dummyImageMemory;
    free(luna_VK_CreateTextureFromDisk("../Assets/error.png", &width, &height, &dummyImageFmt, &ctext->error_image.image, &dummyImageMemory));

    const luna_GPU_SamplerCreateInfo sampler_info = {
        .filter = VK_FILTER_LINEAR,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT
    };
    luna_GPU_CreateSampler(&sampler_info, &ctext->error_sampler);

    const luna_GPU_TextureViewCreateInfo view_info = {
        .format = dummyImageFmt,
        .view_type = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    luna_GPU_CreateTextureView(&view_info, &ctext->error_image);

    const VkDescriptorImageInfo dummyImageInfo = {
        .sampler = ctext->error_sampler.vksampler,
        .imageView = ctext->error_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet write_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ctext->desc_set.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &dummyImageInfo,
    };
    for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++) {
        write_set.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &write_set, 0, NULL);
    }

    const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants) },
    };

    csm_shader_t *vertex, *fragment;
    cassert(csm_load_shader("ctext/vert", &vertex) != -1);
    cassert(csm_load_shader("ctext/frag", &fragment) != -1);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { rd->ctext->desc_set.layout };

    const luna_GPU_PipelineBlendState blend = luna_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = (const struct csm_shader_t *const *)shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;

    luna_GPU_CreatePipelineLayout(&pc, &ctext->pipeline.pipeline_layout);
    pc.pipeline_layout = ctext->pipeline.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &ctext->pipeline.pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND);
}

void ctext_ff_write(const char *path, const cfont_t *fnt)
{
    ctext_ff_header header = {
        .line_height = fnt->line_height,
        .space_width = fnt->space_width,
        .bmpwidth = fnt->atlas.width,
        .bmpheight = fnt->atlas.height,
        .numglyphs = cg_hashmap_size(fnt->glyph_map)
    };

    strncpy(header.path, fnt->path, 128);
    strncpy(header.family_name, fnt->family_name, 128);
    strncpy(header.style_name, fnt->style_name, 128);

    FILE *f = fopen(path, "wb");
    cassert(f != NULL);

    fwrite(&header, sizeof(ctext_ff_header), 1, f);
    ch_node_t **glyph_map_root = cg_hashmap_root_node(fnt->glyph_map);
    for (int i = 0; i < cg_hashmap_capacity(fnt->glyph_map); i++) {
        const ch_node_t *node = glyph_map_root[i];
        if (node && node->is_occupied) {
            const int node_key = *(char *)node->key;
            const ctext_glyph *node_value = (ctext_glyph *)node->value;

            const ctext_glyph_entry entry = {
                .codepoint = node_key,
                .advance = node_value->advance,
                .x0 = node_value->x0,
                .x1 = node_value->x1,
                .y0 = node_value->y0,
                .y1 = node_value->y1,
                .l = node_value->l,
                .b = node_value->b,
                .r = node_value->r,
                .t = node_value->t,
            };
            fwrite(&entry, sizeof(ctext_glyph_entry), 1, f);
        }
    }
    fwrite(fnt->atlas.data, fnt->atlas.width * fnt->atlas.height, 1, f);

    const luna_Image img = (luna_Image){
        .w = fnt->atlas.width,
        .h = fnt->atlas.height,
        .data = fnt->atlas.data,
        .fmt = VK_FORMAT_R8_UNORM
    };
    luna_ImageWritePNG(&img, "PISS.png");
    fclose(f);
}

int ctext_ff_read(const char *path, cfont_t *fnt) {
    // warning, we somehow need to check if the cache is the exact font file the user is requesting for...
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 1;
    }

    ctext_ff_header header;
    fread(&header, sizeof(ctext_ff_header), 1, f);

    bool correct_cache_file = (strncmp(header.path, path, 128) == 0);
    if (!correct_cache_file) {
        // the user is requesting a different font file
        fclose(f);
        return CTEXT_ERR_WRONG_CACHE;
    }

    strncpy(fnt->path, header.path, 128);
    strncpy(fnt->family_name, header.family_name, 128);
    strncpy(fnt->style_name, header.style_name, 128);

    fnt->line_height = header.line_height;
    fnt->space_width = header.space_width;
    fnt->atlas.width = header.bmpwidth;
    fnt->atlas.height = header.bmpheight;

    for (int i = 0; i < header.numglyphs; i++) {
        ctext_glyph_entry entry;
        fread(&entry, sizeof(ctext_glyph_entry), 1, f);

        char glyphi = entry.codepoint;
        ctext_glyph glyph;
        
        glyph.advance = entry.advance;
        glyph.x0 = entry.x0;
        glyph.x1 = entry.x1;
        glyph.y0 = entry.y0;
        glyph.y1 = entry.y1;
        glyph.l = entry.l;
        glyph.b = entry.b;
        glyph.r = entry.r;
        glyph.t = entry.t;
        cg_hashmap_insert(fnt->glyph_map, &glyphi, &glyph);
    }
    fnt->atlas.data = calloc(header.bmpwidth, header.bmpheight);
    fread(fnt->atlas.data, header.bmpwidth * header.bmpheight, 1, f);

    fclose(f);

    return CTEXT_ERR_SUCCESS; // return 0
}

float ctext_get_scale_for_fit(const cfont_t *fnt, const cg_string_t *str, vec2 bbox)
{
    float width, height;
    ctext_get_text_size(fnt, str, &width, &height);

    float scale_x = bbox.x / width;
    float scale_y = bbox.y / height;
    return fminf(scale_x, scale_y);
}