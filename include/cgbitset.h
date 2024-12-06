#ifndef __CBITSET_H
#define __CBITSET_H

#include "defines.h"

typedef struct cg_bitset_t {
    u8 *data;
    int size;
} cg_bitset_t;
typedef unsigned char cg_bitset_bit;

typedef struct cg_bitset_t cg_bitset_t;
cg_bitset_t cg_bitset_init(int init_capacity);
void cg_bitset_set_bit(cg_bitset_t *set, int bitindex);
void cg_bitset_set_bit_to(cg_bitset_t *set, int bitindex, cg_bitset_bit to);
void cg_bitset_clear_bit(cg_bitset_t *set, int bitindex);
void cg_bitset_toggle_bit(cg_bitset_t *set, int bitindex);
cg_bitset_bit cg_bitset_access_bit(cg_bitset_t *set, int bitindex);
void cg_bitset_copy_from(cg_bitset_t *dst, const cg_bitset_t *src);
void cg_bitset_destroy(cg_bitset_t *set);

#endif//__CBITSET_H