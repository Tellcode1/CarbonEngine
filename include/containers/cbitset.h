#ifndef __CBITSET_H
#define __CBITSET_H

typedef unsigned char cbitset_bit;

typedef struct cbitset_t cbitset_t;
cbitset_t *cbitset_init(int init_capacity);
void cbitset_set_bit(cbitset_t *set, int bitindex);
void cbitset_set_bit_to(cbitset_t *set, int bitindex, cbitset_bit to);
void cbitset_clear_bit(cbitset_t *set, int bitindex);
void cbitset_toggle_bit(cbitset_t *set, int bitindex);
cbitset_bit cbitset_access_bit(cbitset_t *set, int bitindex);
void cbitset_destroy(cbitset_t *set);

#endif//__CBITSET_H