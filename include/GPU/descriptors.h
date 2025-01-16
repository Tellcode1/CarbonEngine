#ifndef __LUNA_DESCRIPTORS_H__
#define __LUNA_DESCRIPTORS_H__

#include "../../common/math/math.h"
#include "vkstdafx.h"

// WARNING: Currently only supports the first 11 descriptor types.

typedef struct luna_DescriptorPoolSize {
  VkDescriptorType type;
  int capacity;
  int numchilds; // how many are being used
} luna_DescriptorPoolSize;

typedef struct luna_DescriptorSet luna_DescriptorSet;

typedef struct luna_DescriptorPool {
  VkDescriptorPool pool;
  int max_child_sets;
  luna_DescriptorPoolSize descriptors[11];
  luna_DescriptorSet **sets;
  int nsets;
} luna_DescriptorPool;

typedef struct luna_DescriptorSet {
  int canary;
  VkDescriptorSetLayout layout;
  VkDescriptorSet set;
  luna_DescriptorPool *pool;
  VkWriteDescriptorSet *writes;
  int nwrites;
} luna_DescriptorSet;

extern void luna_DescriptorSetSubmitWrite(luna_DescriptorSet *set, const VkWriteDescriptorSet *write);
extern void luna_DescriptorSetDestroy(luna_DescriptorSet *set);
extern void luna_DescriptorPoolDestroy(luna_DescriptorPool *pool);
extern void __luna_DescriptorPool_Allocate(luna_DescriptorPool *pool);
extern void luna_DescriptorPool_Init(luna_DescriptorPool *dst);
extern void luna_AllocateDescriptorSet(luna_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings,
                                       luna_DescriptorSet **dst);

#endif //__LUNA_DESCRIPTORS_H__