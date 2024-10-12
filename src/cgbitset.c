#include <stdlib.h>
#include "../../include/defines.h"
#include "../../include/cgbitset.h"

typedef struct cg_bitset_t {
    u8 *data;
    int size;
} cg_bitset_t;

cg_bitset_t *cg_bitset_init(int init_capacity)
{
    cg_bitset_t *set = malloc(sizeof(struct cg_bitset_t));
    if (init_capacity > 0) {
        init_capacity = (init_capacity + 7) / 8;
        set->size = init_capacity;
        set->data = malloc(init_capacity * sizeof(uint8_t));
    } else {
        set->size = 0;
    }
    return set;
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

void cg_bitset_destroy(cg_bitset_t *set)
{
    free(set->data);
    free(set);
}
