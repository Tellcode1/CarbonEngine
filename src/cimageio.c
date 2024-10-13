#include "../include/cimage.h"
#include "../include/defines.h"

#include <stdlib.h>

#include <png.h>
#include <jpeglib.h>

const char* get_file_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return "piss";
    return dot + 1;
}

cg_tex2D cimg_load(const char *path)
{
    const char *ext = get_file_extension(path);
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) {
        return cimg_load_jpg(path);
    } else if (strcmp(ext, "png") == 0) {
        return cimg_load_png(path);
    }
    cassert(0);
    return (cg_tex2D){};
}

cg_tex2D cimg_load_png(const char *path)
{
    cg_tex2D texture = {};
    
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
        case 1: texture.fmt = CFMT_R8_UNORM; break;
        case 2: texture.fmt = CFMT_RG8_UNORM; break;
        case 3: texture.fmt = CFMT_RGB8_UNORM; break;
        case 4: texture.fmt = CFMT_RGBA8_UNORM; break;
        default:
            printf("unsupported file(png) format: channels = %d\n", channels);
            cassert(0);
            break;
        break;
    }

    int rowbytes = png_get_rowbytes(png, info);
    texture.data = (unsigned char*)malloc(rowbytes * texture.h * channels);
    cassert(texture.data != NULL);

    uint8_t **row_pointers = malloc(rowbytes * texture.h);
    for (int y = 0; y < texture.h; y++) {
        row_pointers[y] = texture.data + y * rowbytes;
    }
    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);
    free(row_pointers);

    return texture;
}

cg_tex2D cimg_load_jpg(const char *path)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f;
    cg_tex2D img = {};

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
            img.fmt = CFMT_R8_UNORM;
            break;
        case 3:
            img.fmt = CFMT_RGB8_UNORM;
            break;
        default:
            printf("invalid num channels: %d\n", channels);
            cassert(0);
            break;
        break;
    }

    img.data = (unsigned char*)malloc(img.w * img.h * channels);

    unsigned char *bufarr[1];
    for (int i = 0; i < cinfo.output_height; i++) {
        bufarr[0] = img.data + i * img.w * channels;
        jpeg_read_scanlines(&cinfo, bufarr, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(f);

    return img;
}

void cimg_write_png(const cg_tex2D *tex, const char *path)
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
        case CFMT_R8_UNORM:
        case CFMT_R8_SNORM:
        case CFMT_R8_UINT:
        case CFMT_R8_SINT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case CFMT_RG8_UNORM:
        case CFMT_RG8_SNORM:
        case CFMT_RG8_UINT:
        case CFMT_RG8_SINT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case CFMT_RGB8_UNORM:
        case CFMT_RGB8_SNORM:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case CFMT_RGBA8_UNORM:
        case CFMT_RGBA8_SNORM:
        case CFMT_RGBA8_UINT:
        case CFMT_RGBA8_SINT:
        case CFMT_SRGB8_ALPHA8:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case CFMT_R16_UNORM:
        case CFMT_R16_SNORM:
        case CFMT_R16_UINT:
        case CFMT_R16_SINT:
        case CFMT_R16_FLOAT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case CFMT_RG16_UNORM:
        case CFMT_RG16_SNORM:
        case CFMT_RG16_UINT:
        case CFMT_RG16_SINT:
        case CFMT_RG16_FLOAT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case CFMT_RGB16_UNORM:
        case CFMT_RGB16_SNORM:
        case CFMT_RGB16_UINT:
        case CFMT_RGB16_SINT:
        case CFMT_RGB16_FLOAT:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case CFMT_RGBA16_UNORM:
        case CFMT_RGBA16_SNORM:
        case CFMT_RGBA16_UINT:
        case CFMT_RGBA16_SINT:
        case CFMT_RGBA16_FLOAT:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case CFMT_R32_UINT:
        case CFMT_R32_SINT:
        case CFMT_R32_FLOAT:
            coltype = PNG_COLOR_TYPE_GRAY;
            break;
        case CFMT_RG32_UINT:
        case CFMT_RG32_SINT:
        case CFMT_RG32_FLOAT:
            coltype = PNG_COLOR_TYPE_GA;
            break;
        case CFMT_RGB32_UINT:
        case CFMT_RGB32_SINT:
        case CFMT_RGB32_FLOAT:
            coltype = PNG_COLOR_TYPE_RGB;
            break;
        case CFMT_RGBA32_UINT:
        case CFMT_RGBA32_SINT:
        case CFMT_RGBA32_FLOAT:
            coltype = PNG_COLOR_TYPE_RGBA;
            break;
        case CFMT_D16_UNORM:
        case CFMT_D32_FLOAT:
        case CFMT_D24_UNORM_S8_UINT:
        case CFMT_D32_FLOAT_S8_UINT:
        default:
            coltype = -1;
            break;
        break;
    }

    const int bytesperpixel = cfmt_get_bytesperpixel(tex->fmt);
    png_set_IHDR(png, info, tex->w, tex->h, bytesperpixel * 8, coltype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    png_bytep row_pointers[tex->h];
    for (int y = 0; y < tex->h; y++) {
        row_pointers[y] = tex->data + y * tex->w * bytesperpixel;
    }
    png_write_image(png, row_pointers);

    png_write_end(png, NULL);

    fclose(f);
    png_destroy_write_struct(&png, &info);
}