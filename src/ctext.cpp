#include "../../../include/ctext.hpp"
#include "../../../include/cgfx.h"

#include "../../../include/cshadermanager.h"
#include "../../../include/cvk.hpp"
#include "../../../include/vkhelper.hpp"

#define MSDFGEN_PUBLIC // ?
#include "../../../external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

#define MAX(a,b) ({ __typeof__(a) a_ = a; __typeof__(a) b_ = b; (a_ > b_) ? a_ : b_; })

struct cglyph_vertex_t {
    cm::vec3 pos;
    cm::vec2 uv;
};

struct ctext::text_drawcall_t {
    u32 vertex_count;
    u32 index_count;
    u32 index_offset;
    cglyph_vertex_t *vertices;
    u32 *indices;
};

VkDescriptorPool ctext::desc_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout ctext::desc_Layout = VK_NULL_HANDLE;
VkDescriptorSet ctext::desc_set = VK_NULL_HANDLE;
VkImage ctext::error_image = VK_NULL_HANDLE;
VkImageView ctext::error_image_view = VK_NULL_HANDLE;
VkSampler ctext::error_image_sampler = VK_NULL_HANDLE;

/* I have no idea what any of this is */

template <typename T, u32 channels>
u8 *i_hate_my_life(u32 width, u32 height, T &generator, std::vector<msdf_atlas::GlyphGeometry> &glyphs) {
    generator.setThreadCount(16);
    generator.generate(glyphs.data(), glyphs.size());
    msdfgen::BitmapConstRef<f32, channels> bitmap = generator.atlasStorage();

    u8* pixelBuffer = reinterpret_cast<u8*>(malloc(width * height * channels));

    // what the f(*(#$)*@#) is this
    for (u32 y = 0; y < height; y++)
        for (u32 x = 0; x < width; x++)
            for (u32 c = 0; c < channels; c++)
                pixelBuffer[(width * (height - y - 1) + x) * channels + c] = msdfgen::pixelFloatToByte(bitmap.pixels[(width * y + x) * channels + c]);

    return pixelBuffer;
}

void ctext::load_font(crenderer_t *rd, const CFontLoadInfo *__restrict pInfo, cfont_t **dst)
{
	if (!pInfo || !dst)
	{
		LOG_ERROR("pInfo or dst is nullptr!");
		return;
	}

    cassert_and_ret(pInfo->scale > 0.0f);

    msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype();
    if (!ft) {
        LOG_ERROR("msdfgen<Freetype> failed to initialize");
        return;
    }

    msdfgen::FontHandle *fnt = msdfgen::loadFont(ft, pInfo->fontPath);
    if (!fnt) {
        msdfgen::deinitializeFreetype(ft);
        LOG_ERROR("Failed to load fnt. Is path (\"%s\") valid?", pInfo->fontPath);
        return;
    }

    *dst = new cfont_t();

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry(&glyphs);

    fontGeometry.loadCharset(fnt, 1.0f, msdf_atlas::Charset::ASCII);

    constexpr double maxCornerAngle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringByDistance, maxCornerAngle, 0);

    const f32 scale = ceilf(pInfo->scale);
    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setMinimumScale(pInfo->scale);
    packer.setPixelRange(scale);
    packer.setMiterLimit(scale);
    packer.setSpacing(2);
    packer.pack(glyphs.data(), glyphs.size());

    (*dst)->pixel_range = scale;

    i32 width_ = 0, height_ = 0;
    packer.getDimensions(width_, height_);
    u32 width = width_, height = height_; // I hate people who use integers for no reason like bro an unsigned is obviously better here

    const VkFormat bitmap_fmt =
        (pInfo->channel_count == 4) ? VK_FORMAT_R8G8B8A8_UNORM :
        (pInfo->channel_count == 3) ? VK_FORMAT_R8G8B8_UNORM :
        (pInfo->channel_count == 1) ? VK_FORMAT_R8_UNORM :
        VK_FORMAT_UNDEFINED;

    using sdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        1,
        msdf_atlas::sdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 1>
    >;

    using msdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        3,
        msdf_atlas::msdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 3>
    >;

    using mtsdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        4,
        msdf_atlas::mtsdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 4>
    >;

    u8 *pixelBuffer;
    if (pInfo->channel_count == CHANNELS_1) {
        sdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<sdf_gen, 1>(width, height, gen, glyphs);
    }
    else if (pInfo->channel_count == CHANNELS_3) {
        msdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<msdf_gen, 3>(width, height, gen, glyphs);
    }
    else if (pInfo->channel_count == CHANNELS_4) {
        mtsdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<mtsdf_gen, 4>(width, height, gen, glyphs);
    }
    else {
        __builtin_unreachable();
        LOG_AND_ABORT("Invalid channel count");
    }

    (*dst)->glyph_map = chashmap_init(16, sizeof(char), sizeof(CFGlyph), NULL, NULL);
    (*dst)->drawcalls = cvector_init(sizeof(text_drawcall_t), 4);

    for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
        if (glyph.isWhitespace()) {
            (*dst)->space_width = glyph.getAdvance();
            continue;
        }

        f64 x0_, y0_, x1_, y1_;
        glyph.getQuadPlaneBounds(x0_, y0_, x1_, y1_);

        f64 l_, b_, r_, t_;
        glyph.getQuadAtlasBounds(l_, b_, r_, t_);

        f32 l = (f32(l_)) / f32(width);
        f32 r = (f32(r_)) / f32(height);
        f32 b  =(1.0f - (f32)b_) / f32(width);
        f32 t  =(1.0f - (f32)t_) / f32(height);

        f32 x0 = f32(x0_);
        f32 x1 = f32(x1_);
        f32 y0 = (f32)y0_;
        f32 y1 = (f32)y1_;

        CFGlyph mogus;

        const f32 positions[4] = {
            x0, x1,
            y0, y1,
        };

        const f32 uv_positions[4] = {
            l, r,
            b, t
        };

        for (int i = 0; i < 4; i++) {
            mogus.positions[i] = positions[i];
            mogus.uv[i] = uv_positions[i];
        }

        msdfgen::FontMetrics metrics;
        msdfgen::getFontMetrics(metrics, fnt);

        (*dst)->line_height = f32(metrics.lineHeight) / f32(metrics.emSize);

        mogus.advance = static_cast<f32>(glyph.getAdvance());
        const char codepoint = glyph.getCodepoint();
        chashmap_insert((*dst)->glyph_map, &codepoint, &mogus);
    }

    (*dst)->atlas_width = width;
    (*dst)->atlas_height = height;
    (*dst)->atlas_data = pixelBuffer;

    help::Images::killme(pixelBuffer, width, height, bitmap_fmt, pInfo->channel_count, &(*dst)->texture, &(*dst)->texture_memory);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &sampler_info, nullptr, &(*dst)->sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = bitmap_fmt;
    view_info.image = (*dst)->texture;
    view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &view_info, nullptr, &(*dst)->texture_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo error_image_info{};
    error_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    error_image_info.sampler = (*dst)->sampler;
    error_image_info.imageView = (*dst)->texture_view;

    for (u32 i = 0; i < MAX_FONT_COUNT; i++)
    {
        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = desc_set;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &error_image_info;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    }

    msdfgen::destroyFont(fnt);
    msdfgen::deinitializeFreetype(ft);

    VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(cm::vec3) }, // uv
    };

    VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(cm::vec3) + sizeof(cm::vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cm::mat4) + sizeof(f32) },
    };

    csm_shader_t *vertex, *fragment;
    csm_load_shader("ctext/vert", &vertex);

    const char *fshader_path = "ctext/sdf";

    if (pInfo->channel_count == CHANNELS_SDF)
        fshader_path = "ctext/sdf";
    else if (pInfo->channel_count == CHANNELS_MSDF)
        fshader_path = "ctext/msdf";
    else if (pInfo->channel_count == CHANNELS_MTSDF)
        fshader_path = "ctext/mtsdf";

    csm_load_shader(fshader_path, &fragment);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { desc_Layout };

    const cvk_pipeline_blend_state blend = cvk_pipeline_blend_state(CVK_BLEND_PRESET_ALPHA);

    cvk_pipeline_create_info pc{};
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = crenderer_get_render_pass(rd);
    pc.pipeline_layout = (*dst)->pipeline_layout;
    
    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const cengine_extent2d RenderExtent = crenderer_get_render_extent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;
    cvk_create_pipeline_layout(device, &pc, &(*dst)->pipeline_layout);
    pc.pipeline_layout = (*dst)->pipeline_layout;
    cvk_create_graphics_pipeline(device, &pc, &(*dst)->pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND | CVK_PIPELINE_FLAGS_UNFORCE_CULLING);

    vkDestroyShaderModule(device, (*dst)->vshader, nullptr);
    vkDestroyShaderModule(device, (*dst)->fshader, nullptr);
}

void ctext::end_render(crenderer_t *rd, ccamera camera, cfont_t *fnt, cm::mat4 model)
{
    if (!fnt->to_render)
        return;

    const VkCommandBuffer cmd = crenderer_get_drawbuffer(rd);
    constexpr VkDeviceSize offsets[] = { 0 };

    struct push_constants {
        cm::mat4 model;
        f32 scale;
    } pc;

    pc.model = camera.get_projection() * camera.get_view();
    pc.scale = fnt->pixel_range;

    vkCmdPushConstants(
        cmd,
        fnt->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(cm::mat4) + sizeof(f32),
        &pc
    );

    // Viewport && scissor are set by renderer so no need to set them here
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer, offsets);
    vkCmdBindIndexBuffer(cmd, fnt->buffer, fnt->index_buffer_offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline);
    vkCmdDrawIndexed(cmd, fnt->index_count, 1, 0, 0, 0);
}

#include <sstream>

static cvector_t * /* cstring_t * */ split_string(const cstring_t *str) {
    cstring_t *substr = NULL;
    cvector_t *result = NULL;
    int i_start = 0;

    result = cvector_init( sizeof(cstring_t *), 16 );

    // // I got this from reddit.
    for(int i = 0; i < cstring_length(str); i++)
    {
        if(cstring_data(str)[i] == '\n')
        {
            substr = cstring_substring(str, i_start, i - i_start);
            cassert(substr != NULL);
            cvector_push_back( result, &substr );
            i_start = i + 1;
        }
    }

    // add the remaining str
    substr = cstring_substring(str, i_start, cstring_length(str));
    cvector_push_back( result, &substr );

    return result;
}

void ctext::begin_render(cfont_t *fnt)
{
    if (cvector_size(fnt->drawcalls) == 0) {
        fnt->to_render = false;
        return;
    }

    u32 total_vertex_byte_size = 0;
    u32 total_index_count = 0;

    for (int i = 0; i < cvector_size(fnt->drawcalls); i++) {
        const text_drawcall_t &drawcall = ((text_drawcall_t *)cvector_data(fnt->drawcalls))[i];
        total_vertex_byte_size += drawcall.vertex_count * sizeof(cglyph_vertex_t);
        total_index_count += drawcall.index_count;
    }

    const u32 total_index_byte_size = total_index_count * sizeof(u32);
    const u32 total_buffer_size = total_index_byte_size + total_vertex_byte_size;

    fnt->resize_buffer(total_buffer_size, total_vertex_byte_size);

    void *mapped;
    vkMapMemory(device, fnt->buffer_memory, 0, fnt->allocated_size, 0, &mapped);

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (int i = 0; i < cvector_size(fnt->drawcalls); i++) {
        const text_drawcall_t &drawcall = ((text_drawcall_t *)cvector_data(fnt->drawcalls))[i];
        memcpy(static_cast<u8 *>(mapped) + vertex_copy_iterator, drawcall.vertices, drawcall.vertex_count * sizeof(cglyph_vertex_t));
        memcpy(static_cast<u8 *>(mapped) + total_vertex_byte_size + index_copy_iterator, drawcall.indices, drawcall.index_count * sizeof(u32));
        vertex_copy_iterator += drawcall.vertex_count * sizeof(cglyph_vertex_t);
        index_copy_iterator += drawcall.index_count * sizeof(u32);
    }
    vkUnmapMemory(device, fnt->buffer_memory);

    fnt->index_buffer_offset = total_vertex_byte_size;
    fnt->index_count = total_index_count;
    fnt->to_render = true;

    fnt->chars_drawn = 0;
    for (int i = 0; i < cvector_size(fnt->drawcalls); i++) {
        text_drawcall_t &drawcall = ((text_drawcall_t *)cvector_data(fnt->drawcalls))[i];
        free(drawcall.vertices);
    }
    cvector_clear(fnt->drawcalls);
}

static void render_line(
    const ctext::cfont_t *fnt,
    const cstring_t *str,
    const ctext::text_drawcall_t* drawcall,
    const ctext::text_render_info *pInfo,
    const f32 &y,
    const cvector_t * /* ctext::CFGlyph * */ glyph_table,
    const chashmap_t * /* <char, int> */ index_table,
    int &glyph_iter
) {

    const f32 scaled_space_width = fnt->space_width * pInfo->scale;
    const f32 scaled_tab_width = scaled_space_width * 4.0f;

    cglyph_vertex_t* vertex_output_data = drawcall->vertices + glyph_iter * 4;
    u32* index_output_data = drawcall->indices + glyph_iter * 6;
    u32 index_offset = fnt->chars_drawn * 4;

    f32 width = 0.0f;
    int local_glyph_iter = glyph_iter;
    for (int i = 0; i < cstring_length(str); i++) {
        const char c = cstring_data(str)[i];
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
                int index = *(int *)chashmap_find(index_table, &c);
                ctext::CFGlyph *glyph = ((ctext::CFGlyph **)cvector_data(glyph_table))[ index ];
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

    for (int i = 0; i < cstring_length(str); i++) {
        switch (cstring_data(str)[i]) {
            case ' ':
                x += scaled_space_width;
                continue;
            case '\t':
                x += scaled_tab_width;
                continue;
            default:
                const ctext::CFGlyph &glyph = *(((ctext::CFGlyph **)cvector_data(glyph_table))[*(u32 *)chashmap_find(index_table, cstring_data(str) + i)]);

                const f32 glyph_x0 = glyph.positions[0] * pInfo->scale + x;
                const f32 glyph_x1 = glyph.positions[1] * pInfo->scale + x;
                const f32 glyph_y0 = glyph.positions[2] * pInfo->scale + y;
                const f32 glyph_y1 = glyph.positions[3] * pInfo->scale + y;

                const cm::vec2 texture_coordinates[4] = {
                    cm::vec2(glyph.uv[0], glyph.uv[2]),
                    cm::vec2(glyph.uv[1], glyph.uv[2]),
                    cm::vec2(glyph.uv[1], glyph.uv[3]),
                    cm::vec2(glyph.uv[0], glyph.uv[3])
                };

                const f32 scaled_z = pInfo->position.z * pInfo->scale;

                vertex_output_data[0] = (cglyph_vertex_t){ cm::vec3(glyph_x0, glyph_y0, scaled_z), texture_coordinates[0] };
                vertex_output_data[1] = (cglyph_vertex_t){ cm::vec3(glyph_x1, glyph_y0, scaled_z), texture_coordinates[1] };
                vertex_output_data[2] = (cglyph_vertex_t){ cm::vec3(glyph_x1, glyph_y1, scaled_z), texture_coordinates[2] };
                vertex_output_data[3] = (cglyph_vertex_t){ cm::vec3(glyph_x0, glyph_y1, scaled_z), texture_coordinates[3] };

                index_output_data[0] = index_offset;
                index_output_data[1] = index_offset + 1;
                index_output_data[2] = index_offset + 2;
                index_output_data[3] = index_offset + 2;
                index_output_data[4] = index_offset + 3;
                index_output_data[5] = index_offset;

                vertex_output_data += 4;
                index_output_data += 6;
                index_offset += 4;

                x += glyph.advance * pInfo->scale;
                glyph_iter++;
                continue;
        }
    }
}

inline int get_effective_length(const char *buf, int buflen) {
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
    ctext::cfont_t *fnt,
    ctext::text_drawcall_t* drawcall,
    const ctext::text_render_info *pInfo,
    const cstring_t *str,
    const u32 effective_length,
    const cvector_t * /* ctext::CFGlyph * */ glyph_table,
    const chashmap_t * /* <char, int> */ index_table
) {
    if (cstring_length(str) == 0)
        return;

    cvector_t *splitted_strings = split_string(str);

    const f32 scaled_line_height = fnt->line_height * pInfo->scale;

    f32 y = pInfo->position.y;
    const f32 text_size = ((fnt->line_height * pInfo->scale) * cvector_size(splitted_strings));
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
    for (int i = 0; i < cvector_size(splitted_strings); i++) {;
        render_line(fnt, *(cstring_t **)cvector_get(splitted_strings, i), drawcall, pInfo, y + i * scaled_line_height, glyph_table, index_table, glyph_iter);
        fnt->chars_drawn = old_chars_drawn + glyph_iter;
    }

    for (int i = 0; i < cvector_size(splitted_strings); i++) {
        cstring_destroy( ((cstring_t **)cvector_data(splitted_strings))[i] );
    }
    cvector_destroy(splitted_strings);
}

void ctext::render(cfont_t *fnt, const text_render_info *pInfo, const char *fmt, ...)
{
    char buffer[512] = {};

    va_list arg;
    va_start(arg, fmt);
    vsprintf(buffer, fmt, arg);
    va_end(arg);

    const u32 effective_length = get_effective_length(buffer, strlen(buffer));
    if (effective_length == 0) {
        return;
    }

    cstring_t *str = cstring_init_str(buffer);

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;

    const u32 allocation_size = (vertex_count * sizeof(cglyph_vertex_t)) + (index_count * sizeof(u32));

    void *allocation = malloc(allocation_size);

    text_drawcall_t drawcall;
    drawcall.vertices = (cglyph_vertex_t *)allocation;
    drawcall.index_offset = (vertex_count * sizeof(cglyph_vertex_t));
    drawcall.indices = (u32 *)((uchar *)allocation + drawcall.index_offset);

    cvector_t * /* ctext::CFGlyph * */ glyph_table = cvector_init(sizeof(ctext::CFGlyph *), effective_length);
    chashmap_t * /*<char, int>*/ index_table = chashmap_init(effective_length, sizeof(char), sizeof(int), ctext_hash, NULL);

    int pen = 0;
    for (int i = 0; i < cstring_length(str); i++) {
        const char c = cstring_data(str)[i];
        if (c == '\000')
            break;
        else if (c == ' ' || c == '\t' || c == '\n')
            continue;
        else if (chashmap_find(index_table, &c) == NULL) {
            CFGlyph *glyph = (CFGlyph *)chashmap_find(fnt->glyph_map, &c);
            cvector_push_back(glyph_table, &glyph);
            chashmap_insert(index_table, &c, &pen);
            pen++;
        }
    }

    gen_vertices(fnt, &drawcall, pInfo, str, effective_length, glyph_table, index_table);

    cvector_destroy(glyph_table);
    chashmap_destroy(index_table);
    cstring_destroy(str);

    drawcall.vertex_count = effective_length * 4;
    drawcall.index_count = effective_length * 6;

    cvector_push_back(fnt->drawcalls, &drawcall);
}

void ctext::init()
{
    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FONT_COUNT },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &desc_pool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool");

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = array_len(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &desc_Layout) != VK_SUCCESS)
        LOG_ERROR("%s Failto create descriptor set layout");

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = desc_pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &desc_Layout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &desc_set) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets");

    u32 width, height;
    VkFormat dummyImageFmt;

    // No need to store image memory because it won't ever be deleted (probably)
    VkDeviceMemory dummyImageMemory;;
    free(help::Images::killme("../Assets/error.png", &width, &height, &dummyImageFmt, &error_image, &dummyImageMemory));

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &error_image_sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = dummyImageFmt;
    viewInfo.image = error_image;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &error_image_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo dummyImageInfo{};
    dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyImageInfo.sampler = error_image_sampler;
    dummyImageInfo.imageView = error_image_view;

    for (u32 i = 0; i < MAX_FONT_COUNT; i++)
    {
        VkWriteDescriptorSet write_set{};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = desc_set;
        write_set.dstBinding = 0;
        write_set.descriptorCount = 1;
        write_set.pImageInfo = &dummyImageInfo;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
    }
}

void ctext::cfont_t::resize_buffer(u32 new_buffer_size, u32 index_buffer_offset)
{
    u32 new_allocation_size;

    if (allocated_size < new_buffer_size) {
        new_allocation_size = MAX(allocated_size * 2, new_buffer_size);
    }
    else if (new_buffer_size < (allocated_size / 3)) {
        new_allocation_size = MAX(allocated_size / 3, new_buffer_size);
    }
    else
        return;

    vkDeviceWaitIdle(device);

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, buffer_memory, nullptr);

    help::Buffers::CreateBuffer(
        new_allocation_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &buffer,
        &buffer_memory
    );

    allocated_size = new_allocation_size;
}
