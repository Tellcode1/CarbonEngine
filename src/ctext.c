#include "../include/ctext.h"
#include "../include/cgfx.h"

#include "../include/cshadermanager.h"
#include "../include/cvk.h"
#include "../include/vkhelper.h"
#include "../include/cimage.h"

#include "../include/cgfxdef.h"

#include "../include/cgfxpipeline.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define MAX(a,b) ({ __typeof__(a) a_ = a; __typeof__(a) b_ = b; (a_ > b_) ? a_ : b_; })

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
} ctext_text_drawcall_t;

/* I have no idea what any of this is */

void ctext_load_font(crenderer_t *rd, const ctext_font_load_info *__restrict pInfo, cfont_t **dst)
{
	if (!pInfo || !dst)
	{
		LOG_ERROR("pInfo or dst is NULL!");
		return;
	}

    cassert_and_ret(pInfo->scale > 0.0f);

    *dst = calloc(1, sizeof(cfont_t));
    const f32 scale = ceilf(pInfo->scale);
    (*dst)->pixel_range = scale;
    (*dst)->glyph_map = cg_hashmap_init(16, sizeof(char), sizeof(ctext_glyph), NULL, NULL);
    (*dst)->drawcalls = cg_vector_init(sizeof(ctext_text_drawcall_t), 4);

    FT_Library lib; FT_Face face;
    if (FT_Init_FreeType(&lib))
    {
        LOG_ERROR("failed to initialize ft\n");
    }

    if (FT_New_Face(lib, pInfo->fontPath, 0, &face))
    {
        LOG_ERROR("failed to load font file: %s\n", pInfo->fontPath);
        FT_Done_FreeType(lib);
    }
    FT_Set_Pixel_Sizes(face, 0, 256);

    if (face->size->metrics.height) {
        (*dst)->line_height = -face->size->metrics.height / (f32)face->units_per_EM;
    } else {
        // ! This doesn't work. find out why
        (*dst)->line_height = -(f32)(face->ascender - face->descender) / face->size->metrics.y_scale;
    }
    (*dst)->atlas = catlas_init(4096, 4096);

    // ! WHAT THE FU IS THIS????
    int xpositions[ 256 ];
    int ypositions[ 256 ];
    for (int i = 0; i < 256; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, i);
        if (glyph_index == 0) {
            continue;
        }
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
            continue;
        }

        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        FT_GlyphSlot g = face->glyph;

        const int w = g->bitmap.width;
        const int h = g->bitmap.rows;
        const unsigned char *buffer = g->bitmap.buffer;

        int x,y;
        if (catlas_add_image(&(*dst)->atlas, w, h, buffer, &x, &y)) {
            printf("atlas error\n");
            continue;
        }
        xpositions[i] = x;
        ypositions[i] = y;
    }

    for (int i = 0; i < 256; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, i);
        if (glyph_index == 0) {
            continue;
        }
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY)) {
            continue;
        }

        if (i == ' ') {
            (*dst)->space_width = face->glyph->advance.x / (f32)face->units_per_EM;
            continue;
        }
        const int w = face->glyph->bitmap.width, h = face->glyph->bitmap.rows;
        const int x = xpositions[i], y = ypositions[i];

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

            .l = (float)(x) / (*dst)->atlas.width,
            .r = (float)(x + w) / (*dst)->atlas.width,
            .b = (float)(y + h) / (*dst)->atlas.height,
            .t = (float)(y) / (*dst)->atlas.height,

            .advance = (f32)face->glyph->metrics.horiAdvance / (f32)face->units_per_EM
        };

        const char glyphi = i;
        cg_hashmap_insert((*dst)->glyph_map, &glyphi, &retglyph);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);

    cg_tex2D tex = {};
    tex.w = (*dst)->atlas.width;
    tex.h = (*dst)->atlas.height;
    tex.fmt = CFMT_R8_UINT;
    tex.data = (*dst)->atlas.data;
    cimg_write_png(&tex, "skdlfj.png");

    cgfx_gpu_image_create_info image_info = {
        .format = CFMT_R8_UNORM,
        .samples = CG_SAMPLE_COUNT_1_SAMPLES,
        .type = VK_IMAGE_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .extent = (VkExtent3D){ .width = (*dst)->atlas.width, .height = (*dst)->atlas.height, .depth = 1 },
        .arraylayers = 1,
        .miplevels = 1
    };
    cgfx_gpu_create_image(&image_info, &(*dst)->texture);

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, (*dst)->texture.image, &imageMemoryRequirements);

    cgfx_gpu_memory_allocate(imageMemoryRequirements.size, CGFX_MEMORY_USAGE_GPU_LOCAL, &(*dst)->texture_mem);
    cgfx_gpu_memory_bind_image(&(*dst)->texture_mem, 0, &(*dst)->texture);

    vkh_stage_image_transfer((*dst)->texture.image, (*dst)->atlas.data, (*dst)->atlas.width, (*dst)->atlas.height, 1);

    const cgfx_gpu_sampler_create_info sampler_info = {
        .filter = VK_FILTER_LINEAR,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT
    };
    cgfx_gpu_create_sampler(&sampler_info, &(*dst)->sampler);

    cgfx_gpu_image_view_create_info view_info = {
        .format = CFMT_R8_UNORM,
        .view_type = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    cgfx_gpu_create_image_view(&view_info, &(*dst)->texture);

    const VkDescriptorImageInfo ctext_error_image_info = {
        .sampler = (*dst)->sampler.vksampler,
        .imageView = (*dst)->texture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeSet = {};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = rd->ctext->desc_set;
    writeSet.dstBinding = 0;
    writeSet.descriptorCount = 1;
    writeSet.pImageInfo = &ctext_error_image_info;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    for (u32 i = 0; i < CTEXT_MAX_FONT_COUNT; i++)
    {
        writeSet.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, NULL);
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
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) + sizeof(f32) },
    };

    csm_shader_t *vertex, *fragment;
    csm_load_shader("ctext/vert", &vertex);
    csm_load_shader("ctext/frag", &fragment);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { rd->ctext->desc_Layout };

    const cvk_pipeline_blend_state blend = cvk_init_pipeline_blend_state(CVK_BLEND_PRESET_ALPHA);

    cvk_pipeline_create_info pc = cvk_init_pipeline_create_info();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = crd_get_render_pass(rd);
    pc.pipeline_layout = (*dst)->pipeline_layout;
    
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

    const cg_extent2d RenderExtent = crd_get_render_extent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;
    cvk_create_pipeline_layout(&pc, &(*dst)->pipeline_layout);
    pc.pipeline_layout = (*dst)->pipeline_layout;
    cvk_create_graphics_pipeline(&pc, &(*dst)->pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND | CVK_PIPELINE_FLAGS_UNFORCE_CULLING);
}

void ctext_end_render(crenderer_t *rd, ccamera *camera, cfont_t *fnt, mat4 model)
{
    if (!fnt->to_render)
        return;

    const VkCommandBuffer cmd = crd_get_drawbuffer(rd);
    const VkDeviceSize offsets[] = { 0 };

    struct push_constants {
        mat4 model;
        f32 scale;
    } pc;

    pc.model = m4mul(cam_get_projection(camera), cam_get_view(camera));
    pc.scale = fnt->pixel_range;

    vkCmdPushConstants(
        cmd,
        fnt->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mat4) + sizeof(f32),
        &pc
    );

    // Viewport && scissor are set by renderer so no need to set them here
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline_layout, 0, 1, &rd->ctext->desc_set, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer.vkbuffer, offsets);
    vkCmdBindIndexBuffer(cmd, fnt->buffer.vkbuffer, fnt->index_buffer_offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline);
    vkCmdDrawIndexed(cmd, fnt->index_count, 1, 0, 0, 0);
}

static cg_vector_t split_string(const cg_string_t *str) {
    cg_string_t *substr = NULL;
    cg_vector_t result;
    int i_start = 0;

    result = cg_vector_init(sizeof(cg_string_t *), 16);

    for (int i = 0; i < cg_string_length(str); i++) {
        if (cg_string_data(str)[i] == '\n') {
            if (i > i_start) {
                substr = cg_string_substring(str, i_start, i - i_start);
                cassert(substr != NULL);
                cg_vector_push_back(&result, &substr);
            }
            i_start = i + 1;
        }
    }

    if (i_start < cg_string_length(str)) {
        substr = cg_string_substring(str, i_start, cg_string_length(str) - i_start);
        cassert(substr != NULL);
        cg_vector_push_back(&result, &substr);
    }

    return result;
}

void ctext_begin_render(crenderer_t *rd, cfont_t *fnt)
{
    if (cg_vector_size(&fnt->drawcalls) == 0) {
        fnt->to_render = false;
        return;
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

    ctext__font_resize_buffer(fnt, total_buffer_size, total_vertex_byte_size);

    void *mapped;
    vkMapMemory(device, fnt->buffer_mem.memory, 0, fnt->allocated_size, 0, &mapped);

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        const ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        memcpy((u8 *)(mapped) + vertex_copy_iterator, drawcall->vertices, drawcall->vertex_count * sizeof(cglyph_vertex_t));
        memcpy((u8 *)(mapped) + total_vertex_byte_size + index_copy_iterator, drawcall->indices, drawcall->index_count * sizeof(u32));
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

static void render_line(
    const cfont_t *fnt,
    const cg_string_t *str,
    const ctext_text_drawcall_t* drawcall,
    const ctext_text_render_info *pInfo,
    const f32 y,
    const cg_vector_t * /* ctext_glyph * */ glyph_table,
    const cg_hashmap_t * /* <char, int> */ index_table,
    int *glyph_iter
) {

    const f32 scaled_space_width = fnt->space_width * pInfo->scale;
    const f32 scaled_tab_width = scaled_space_width * 4.0f;

    cglyph_vertex_t* vertex_output_data = drawcall->vertices + (*glyph_iter) * 4;
    u32* index_output_data = drawcall->indices + (*glyph_iter) * 6;
    u32 index_offset = fnt->chars_drawn * 4;

    f32 width = 0.0f;
    int local_glyph_iter = *glyph_iter;
    const int len = cg_string_length(str);
    for (int i = 0; i < len; i++) {
        const char c = cg_string_data(str)[i];
        switch (c) {
            case ' ':
                width += scaled_space_width;
                continue;
            case '\t':
                width += scaled_tab_width;
                continue;
            case '\n':
                continue;
            default:
                int *indexptr = (int *)cg_hashmap_find(index_table, &c);
                int index = *indexptr;
                ctext_glyph *glyph = ((ctext_glyph **)cg_vector_data(glyph_table))[ index ];
                width += glyph->advance * pInfo->scale;
                local_glyph_iter++;
                continue;
        }
    }

    f32 x =
    (pInfo->horizontal == CTEXT_HORI_ALIGN_CENTER) ?
                pInfo->position.x - width / 2.0f
                :
                (pInfo->horizontal == CTEXT_HORI_ALIGN_RIGHT) ?
                    pInfo->position.x - width
                    :
                    pInfo->position.x;

    for (int i = 0; i < len; i++) {
        switch (cg_string_data(str)[i]) {
            case ' ':
                x += scaled_space_width;
                continue;
            case '\t':
                x += scaled_tab_width;
                continue;
            default:
                const ctext_glyph *glyph = (const ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &(cg_string_data(str)[i]));
                const f32 glyph_x0 = glyph->x0 * pInfo->scale + x;
                const f32 glyph_x1 = glyph->x1 * pInfo->scale + x;
                const f32 glyph_y0 = glyph->y0 * pInfo->scale + y;
                const f32 glyph_y1 = glyph->y1 * pInfo->scale + y;

                const f32 scaled_z = pInfo->position.z * pInfo->scale;

                vertex_output_data[0] = (cglyph_vertex_t){ (vec3){glyph_x0, glyph_y0, scaled_z}, (vec2){glyph->l, glyph->b}, };
                vertex_output_data[1] = (cglyph_vertex_t){ (vec3){glyph_x1, glyph_y0, scaled_z}, (vec2){glyph->r, glyph->b}, };
                vertex_output_data[2] = (cglyph_vertex_t){ (vec3){glyph_x1, glyph_y1, scaled_z}, (vec2){glyph->r, glyph->t}, };
                vertex_output_data[3] = (cglyph_vertex_t){ (vec3){glyph_x0, glyph_y1, scaled_z}, (vec2){glyph->l, glyph->t} };

                index_output_data[0] = index_offset;
                index_output_data[1] = index_offset + 1;
                index_output_data[2] = index_offset + 2;
                index_output_data[3] = index_offset + 2;
                index_output_data[4] = index_offset + 3;
                index_output_data[5] = index_offset;

                vertex_output_data += 4;
                index_output_data += 6;
                index_offset += 4;

                x += glyph->advance * pInfo->scale;
                (*glyph_iter)++;
                continue;
        }
    }
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
    const u32 effective_length,
    const cg_vector_t * /* ctext_glyph * */ glyph_table,
    const cg_hashmap_t * /* <char, int> */ index_table
) {
    if (cg_string_length(str) == 0)
        return;

    cg_vector_t splitted_strings = split_string(str);

    const f32 scaled_line_height = fnt->line_height * pInfo->scale;

    f32 y = pInfo->position.y;
    const f32 text_size = ((fnt->line_height * pInfo->scale) * cg_vector_size(&splitted_strings));
    switch(pInfo->vertical)
    {
        case CTEXT_VERT_ALIGN_CENTER:
            y = y - text_size / 2.0f;
            break;

        case CTEXT_VERT_ALIGN_BOTTOM:
            y = y - text_size;
            break;

        case CTEXT_VERT_ALIGN_TOP:
            break;

        default:
            __builtin_unreachable();
            LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)\n", pInfo->vertical);
            break;
        break;
    }

    const i32 old_chars_drawn = fnt->chars_drawn;
    int glyph_iter = 0;
    for (int i = 0; i < cg_vector_size(&splitted_strings); i++) {;
        render_line(fnt, *(cg_string_t **)cg_vector_get(&splitted_strings, i), drawcall, pInfo, y + i * scaled_line_height, glyph_table, index_table, &glyph_iter);
        fnt->chars_drawn = old_chars_drawn + glyph_iter;
    }

    for (int i = 0; i < cg_vector_size(&splitted_strings); i++) {
        cg_string_destroy( ((cg_string_t **)cg_vector_data(&splitted_strings))[i] );
    }
    cg_vector_destroy(&splitted_strings);
}

void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char buffer[BUFSIZ];
    int num = vsnprintf(buffer, BUFSIZ, fmt, arg);
    va_end(arg);

    const u32 effective_length = get_effective_length(buffer, num);
    if (effective_length == 0) {
        return;
    }

    cg_string_t *str = cg_string_init_str(buffer);

    // free(buf);

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;

    const u32 allocation_size = (vertex_count * sizeof(cglyph_vertex_t)) + (index_count * sizeof(u32));

    void *allocation = malloc(allocation_size);

    ctext_text_drawcall_t drawcall = {};
    drawcall.vertices = (cglyph_vertex_t *)allocation;
    drawcall.index_offset = (vertex_count * sizeof(cglyph_vertex_t));
    drawcall.indices = (u32 *)((uchar *)allocation + drawcall.index_offset);

    cg_vector_t /* ctext_glyph * */ glyph_table = cg_vector_init(sizeof(ctext_glyph *), effective_length);
    cg_hashmap_t * /*<char, int>*/ index_table = cg_hashmap_init(effective_length, sizeof(char), sizeof(int), NULL, NULL);

    int pen = 0;
    for (int i = 0; i < cg_string_length(str); i++) {
        const char c = cg_string_data(str)[i];
        if (c == '\000')
            break;
        else if (c == ' ' || c == '\t' || c == '\n')
            continue;
        else if (cg_hashmap_find(index_table, &c) == NULL) {
            const ctext_glyph *glyph = (ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &c);
            cg_vector_push_back(&glyph_table, &glyph);
            cg_hashmap_insert(index_table, &c, &pen);
            pen++;
        }
    }

    gen_vertices(fnt, &drawcall, pInfo, str, effective_length, &glyph_table, index_table);

    cg_vector_destroy(&glyph_table);
    cg_hashmap_destroy(index_table);
    cg_string_destroy(str);

    drawcall.vertex_count = effective_length * 4;
    drawcall.index_count = effective_length * 6;

    cg_vector_push_back(&fnt->drawcalls, &drawcall);
}

void ctext_init(struct crenderer_t *rd)
{
    rd->ctext = calloc(1, sizeof(cg_ctext_module));
    cg_ctext_module *ctext = rd->ctext;

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        (VkDescriptorSetLayoutBinding){ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CTEXT_MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };

    const VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        (VkDescriptorPoolSize){ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CTEXT_MAX_FONT_COUNT },
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, NULL, &ctext->desc_pool) != VK_SUCCESS) {
        LOG_ERROR("Failed to create descriptor pool");
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = array_len(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, NULL, &ctext->desc_Layout) != VK_SUCCESS) {
        LOG_ERROR("%s Failto create descriptor set layout");
    }

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = ctext->desc_pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &ctext->desc_Layout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &ctext->desc_set) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate descriptor sets");
    }

    u32 width, height;
    VkFormat dummyImageFmt;

    // No need to store image memory because it won't ever be deleted (probably)
    // ! you should probably store it and free it.
    VkDeviceMemory dummyImageMemory;
    free(vkh_image_from_disk("../Assets/error.png", &width, &height, &dummyImageFmt, &ctext->error_image.image, &dummyImageMemory));

    const cgfx_gpu_sampler_create_info sampler_info = {
        .filter = VK_FILTER_LINEAR,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT
    };
    cgfx_gpu_create_sampler(&sampler_info, &ctext->error_sampler);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = dummyImageFmt;
    viewInfo.image = ctext->error_image.image;
    viewInfo.components = (VkComponentMapping){ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, NULL, &ctext->error_image.view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image view");
    }

    const VkDescriptorImageInfo dummyImageInfo = {
        .sampler = ctext->error_sampler.vksampler,
        .imageView = ctext->error_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = ctext->desc_set;
    write_set.dstBinding = 0;
    write_set.descriptorCount = 1;
    write_set.pImageInfo = &dummyImageInfo;
    write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++) {
        write_set.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &write_set, 0, NULL);
    }
}

void ctext__font_resize_buffer(cfont_t *fnt,  u32 new_buffer_size, u32 index_buffer_offset)
{
    u32 new_allocation_size;

    if (fnt->allocated_size < new_buffer_size) {
        new_allocation_size = MAX(fnt->allocated_size * 2, new_buffer_size);
    }
    else if (new_buffer_size < (fnt->allocated_size / 3)) {
        new_allocation_size = MAX(fnt->allocated_size / 3, new_buffer_size);
    }
    else
        return;

    vkDeviceWaitIdle(device);
    
    cgfx_gpu_buffer_free(&fnt->buffer);
    cgfx_gpu_memory_free(&fnt->buffer_mem);

    cgfx_gpu_memory_allocate(new_allocation_size, CGFX_MEMORY_USAGE_GPU_LOCAL | CGFX_MEMORY_USAGE_CPU_WRITEABLE, &fnt->buffer_mem);

    cgfx_gpu_create_buffer(
        new_allocation_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &fnt->buffer
    );
    cgfx_gpu_memory_bind_buffer(&fnt->buffer_mem, 0, &fnt->buffer);

    fnt->allocated_size = new_allocation_size;
}
