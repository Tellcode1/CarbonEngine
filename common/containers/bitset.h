#ifndef __NOVA_BITSET_H__
#define __NOVA_BITSET_H__

#include "../mem.h"
#include "../stdafx.h"

typedef struct NV_bitset_t
{
  u8*            data;
  size_t         size;
  NVAllocator* allocator;
} NV_bitset_t;
typedef unsigned char      NV_bitset_bit;

typedef struct NV_bitset_t NV_bitset_t;

NV_bitset_t                NV_bitset_init(int init_capacity);
void                       NV_bitset_set_bit(NV_bitset_t* set, int bitindex);
void                       NV_bitset_set_bit_to(NV_bitset_t* set, int bitindex, NV_bitset_bit to);
void                       NV_bitset_clear_bit(NV_bitset_t* set, int bitindex);
void                       NV_bitset_toggle_bit(NV_bitset_t* set, int bitindex);
NV_bitset_bit              NV_bitset_access_bit(NV_bitset_t* set, int bitindex);
void                       NV_bitset_copy_from(NV_bitset_t* dst, const NV_bitset_t* src);
void                       NV_bitset_destroy(NV_bitset_t* set);

#endif //__NOVA_BITSET_H__