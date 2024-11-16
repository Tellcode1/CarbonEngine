#include "../include/lunaImage.h"
#include "../include/defines.h"

#include <stdlib.h>

#include <png.h>
#include <jpeglib.h>

const char* get_file_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "piss";
    return dot + 1;
}

luna_Image luna_ImageLoad(const char *path)
{
    const char *ext = get_file_extension(path);
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) {
        return luna_ImageLoadJPEG(path);
    } else if (strcmp(ext, "png") == 0) {
        return luna_ImageLoadPNG(path);
    }
    cassert(0);
    return (luna_Image){};
}

luna_Image luna_ImageLoadPNG(const char *path)
{
    luna_Image texture = {};
    
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
        case 1: texture.fmt = VK_FORMAT_R8_UNORM; break;
        case 2: texture.fmt = VK_FORMAT_R8G8_UNORM; break;
        case 3: texture.fmt = VK_FORMAT_R8G8B8_UNORM; break;
        case 4: texture.fmt = VK_FORMAT_R8G8B8A8_UNORM; break;
        default:
            printf("unsupported file(png) format: channels = %d\n", channels);
            cassert(0);
            break;
        break;
    }

    int rowbytes = png_get_rowbytes(png, info);
    texture.data = (unsigned char*)malloc(rowbytes * texture.h * channels);
    cassert(texture.data != NULL);

    u8 **row_pointers = malloc(sizeof(u8 *) * texture.h);
    for (int y = 0; y < texture.h; y++) {
        // ! Figure out the bpp for vk format :D
        row_pointers[y] = texture.data + (texture.h - 1 - y) * texture.w * channels; // REPLACE
    }

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);
    free(row_pointers);

    return texture;
}

luna_Image luna_ImageLoadJPEG(const char *path)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f;
    luna_Image img = {};

    if ((f = fopen(path, "rb")) == NULL) {
        fprintf(stderr, "cimageload :: couldn't open file \"%s\" Are you sure that it exists?\n", path);
        return img;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, 1);

    jpeg_start_decompress(&cinfo);

    img.w = cinfo.output_width;
    img.h = cinfo.output_height;
    
    const int channels = cinfo.output_components;

    switch (channels) {
        case 1:
            img.fmt = VK_FORMAT_R8_UNORM;
            break;
        case 3:
            img.fmt = VK_FORMAT_R8G8B8_UNORM;
            break;
        default:
            printf("invalid num channels: %d\n", channels);
            cassert(0);
            break;
        break;
    }

    img.data = (unsigned char*)malloc(img.w * img.h * channels);

    unsigned char *bufarr[1];
    for (int i = 0; i < (int)cinfo.output_height; i++) {
        bufarr[0] = img.data + i * img.w * channels;
        jpeg_read_scanlines(&cinfo, bufarr, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);

    return img;
}

void luna_ImageWritePNG(const luna_Image *tex, const char *path)
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
    switch (tex->fmt) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        default:
            coltype = -1;
            break;
        break;
    }

    const int bytesperpixel = vk_fmt_get_bpp(tex->fmt);
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