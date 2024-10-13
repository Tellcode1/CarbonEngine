#include <stdlib.h>
#include "../../include/defines.h"
#include "../../include/cgbitset.h"

cg_bitset_t cg_bitset_init(int init_capacity)
{   
    cg_bitset_t ret = {};
    if (init_capacity > 0) {
        init_capacity = (init_capacity + 7) / 8;
        ret.size = init_capacity;
        ret.data = calloc(init_capacity, sizeof(uint8_t));
    } else {
        ret.size = 0;
    }
    return ret;
}

void cg_bitset_set_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] |= (1 << (bitindex % 8));
}

void cg_bitset_set_bit_to(cg_bitset_t *set, int bitindex, cg_bitset_bit to)
{
    to ?
        cg_bitset_set_bit(set, bitindex)
        :
        cg_bitset_clear_bit(set, bitindex);
}

void cg_bitset_clear_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] &= ~(1 << (bitindex % 8));
}

void cg_bitset_toggle_bit(cg_bitset_t *set, int bitindex)
{
    set->data[bitindex / 8] ^= (1 << (bitindex % 8));
}

cg_bitset_bit cg_bitset_access_bit(cg_bitset_t *set, int bitindex)
{
    return (set->data[bitindex / 8] & (1 << (bitindex % 8))) != 0;
}

void cg_bitset_copy_from(cg_bitset_t *dst, cg_bitset_t *src)
{
    if (src->size != dst->size && dst->data) {
        free(dst->data);
        dst->data = malloc(src->size);
        dst->size = src->size;
    }
    // FOR TESTING MOVE THE DATA
    // !FIXME: revert
    memmove(dst->data, src->data, src->size);
}

void cg_bitset_destroy(cg_bitset_t *set)
{
    free(set->data);
    free(set);
}
