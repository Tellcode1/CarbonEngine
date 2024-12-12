#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum RM_ResourceType {
    RM_RESOURCE_TYPE_INT = 0,
    RM_RESOURCE_TYPE_FLOAT = 1,
    RM_RESOURCE_TYPE_TEXTURE = 2,
    RM_RESOURCE_TYPE_OTHER = 2,
} RM_ResourceType;

typedef union RM_ResourceData {
    void *ptr;
    int num;
    float flt;
} RM_ResourceData;

typedef struct RM_Resource RM_Resource;

typedef void (*RM_CreateResource_fn)( RM_Resource *rce, void *pdata );
typedef void (*RM_DestroyResource_fn)( RM_Resource *rce, void *pdata );

typedef struct RM_Resource {
    RM_ResourceType type;
    RM_ResourceData data;
} RM_Resource;

extern int  RM_Init();
extern void RM_Quit();

#ifdef __cplusplus
    }
#endif

#endif//__RESOURCE_MANAGER_H__