#ifndef __C_TEXTURES_HPP__
#define __C_TEXTURES_HPP__

#include "stdafx.hpp"

struct ctex2D {
    const u8 *get_image_data() const {
        return image_data;
    }

    i32 get_width() const {
        return width;
    }

    i32 get_height() const {
        return num_rows;
    }

    i32 get_channel_count() const {
        return channel_count;
    }

    i32 get_size_in_pixels() const {
        return width * num_rows;
    }

    void set_width(i32 w) {
        width = w;
    }

    void set_height(i32 h) {
        num_rows = h;
    }

    void set_channel_count(i32 c) {
        channel_count = c;
    }

    void set_image_data(u8 *ptr) {
        image_data = ptr;
    }

    const f32 *sample(i32 x, i32 y) {
        return (f32 *)(image_data + x + y * num_rows);
    }

    private:
    u8 *image_data;
    i32 width;
    i32 num_rows;
    i8 channel_count;
};

#endif//__C_TEXTURES_HPP__