#include <stdlib.h>
#include "../../include/defines.h"
#include "../../include/containers/cbitset.h"

typedef struct cbitset_t {
    u8 *data;
    int size;
} cbitset_t;

cbitset_t *cbitset_init(int init_capacity)
{
    cbitset_t *set = malloc(sizeof(struct cbitset_t));
    if (init_capacity > 0) {
        init_capacity = (init_capacity + 7) / 8;
        set->size = init_capacity;
        set->data = malloc(init_capacity * sizeof(uint8_t));
    } else {
        set->size = 0;
    }
    return set;
}

void cbitset_set_bit(cbitset_t *set, int bitindex)
{
    set->data[bitindex / 8] |= (1 << (bitindex % 8));
}

void cbitset_set_bit_to(cbitset_t *set, int bitindex, cbitset_bit to)
{
    to ?
        cbitset_set_bit(set, bitindex)
        :
        cbitset_clear_bit(set, bitindex);
}

void cbitset_clear_bit(cbitset_t *set, int bitindex)
{
    set->data[bitindex / 8] &= ~(1 << (bitindex % 8));
}

void cbitset_toggle_bit(cbitset_t *set, int bitindex)
{
    set->data[bitindex / 8] ^= (1 << (bitindex % 8));
}

cbitset_bit cbitset_access_bit(cbitset_t *set, int bitindex)
{
    return (set->data[bitindex / 8] & (1 << (bitindex % 8))) != 0;
}

void cbitset_destroy(cbitset_t *set)
{
    free(set->data);
    free(set);
}
