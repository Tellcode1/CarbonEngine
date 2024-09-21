#ifndef __C_BITSET_HPP__
#define __C_BITSET_HPP__

#include "../defines.h"
#include "../stdafx.h"

template <u32 _size>
struct cbitset {
    u8 data[(_size + 7) / 8] = {0};

    void set(u32 pos) {
        if (pos >= _size) LOG_ERROR("set: access out of range %u", pos);
        data[pos / 8] |= (1 << (pos % 8));
    }

    void set_value(u32 pos, bool value) {
        if (pos >= _size) LOG_ERROR("set_value: access out of range %u", pos);
        value ?
            set(pos)
            :
            clear(pos);
    }

    void clear(u32 pos) {
        if (pos >= _size) LOG_ERROR("clear: access out of range %u", pos);
        data[pos / 8] &= ~(1 << (pos % 8));
    }

    void toggle(u32 pos) {
        if (pos >= _size) LOG_ERROR("toggle: access out of range %u", pos);
        data[pos / 8] ^= (1 << (pos % 8));
    }

    bool test(u32 pos) const {
        if (pos >= _size) LOG_ERROR("test: access out of range %u", pos);
        return (data[pos / 8] & (1 << (pos % 8))) != 0;
    }

    const inline u32 size() const {
        return _size;
    }
};

#endif//__C_BITSET_HPP__