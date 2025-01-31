#ifndef __NOVA_DESCRIPTORS_H__
#define __NOVA_DESCRIPTORS_H__

#include "../../common/math/math.h"
#include "../../common/stdafx.h"
#include "../../external/volk/volk.h"
#include "vkstdafx.h"

NOVA_HEADER_START;

// WARNING: Currently only supports the first 11 descriptor types.

typedef struct NV_descriptor_poolSize {
  uint32_t type;
  int capacity;
  int numchilds; // how many are being used
} NV_descriptor_poolSize;

typedef struct NV_descriptor_set NV_descriptor_set;

typedef struct NV_descriptor_pool {
  VkDescriptorPool pool;
  int max_child_sets;
  NV_descriptor_poolSize descriptors[11];
  NV_descriptor_set **sets;
  int nsets;
} NV_descriptor_pool;

typedef struct NV_descriptor_set {
  int canary;
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
  NV_descriptor_pool *pool;
  struct VkWriteDescriptorSet *writes;
  int nwrites;
} NV_descriptor_set;

extern void NV_descriptor_setSubmitWrite(NV_descriptor_set *set, const VkWriteDescriptorSet *write);
extern void NV_descriptor_setDestroy(NV_descriptor_set *set);
extern void NV_descriptor_poolDestroy(NV_descriptor_pool *pool);
extern void _NV_descriptor_pool_Allocate(NV_descriptor_pool *pool);
extern void NV_descriptor_pool_Init(NV_descriptor_pool *dst);
extern void NV_AllocateDescriptorSet(NV_descriptor_pool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings,
                                       NV_descriptor_set **dst);

NOVA_HEADER_END;

#endif //__NOVA_DESCRIPTORS_H__