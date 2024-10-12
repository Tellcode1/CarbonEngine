#ifndef __CRBTREE_H
#define __CRBTREE_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct crbnode_t crbnode_t;
typedef struct crbtree_t crbtree_t;

extern crbtree_t *crbt_init(int datasize);
extern crbnode_t *crbt_insert(crbtree_t *tree, const void *value);

#ifdef __cplusplus
    }
#endif

#endif//__CRBTREE_H