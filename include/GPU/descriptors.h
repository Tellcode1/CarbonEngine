#ifndef __NOVA_DESCRIPTORS_H__
#define __NOVA_DESCRIPTORS_H__

#include "../../common/math/math.h"
#include "../../common/stdafx.h"
#include "../../external/volk/volk.h"
#include "vkstdafx.h"

NOVA_HEADER_START;

// WARNING: Currently only supports the first 11 descriptor types.

typedef struct NV_DescriptorPoolSize {
  uint32_t type;
  int capacity;
  int numchilds; // how many are being used
} NV_DescriptorPoolSize;

typedef struct NV_DescriptorSet NV_DescriptorSet;

typedef struct NV_DescriptorPool {
  VkDescriptorPool pool;
  int max_child_sets;
  NV_DescriptorPoolSize descriptors[11];
  NV_DescriptorSet **sets;
  int nsets;
} NV_DescriptorPool;

typedef struct NV_DescriptorSet {
  int canary;
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
  NV_DescriptorPool *pool;
  struct VkWriteDescriptorSet *writes;
  int nwrites;
} NV_DescriptorSet;

extern void NV_DescriptorSetSubmitWrite(NV_DescriptorSet *set, const VkWriteDescriptorSet *write);
extern void NV_DescriptorSetDestroy(NV_DescriptorSet *set);
extern void NV_DescriptorPoolDestroy(NV_DescriptorPool *pool);
extern void __NV_DescriptorPool_Allocate(NV_DescriptorPool *pool);
extern void NV_DescriptorPool_Init(NV_DescriptorPool *dst);
extern void NV_AllocateDescriptorSet(NV_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings,
                                       NV_DescriptorSet **dst);

NOVA_HEADER_END;

#endif //__NOVA_DESCRIPTORS_H__