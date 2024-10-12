// don't you dare...
#ifndef __CBTREE_H
#define __CBTREE_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef unsigned char cbtree_bool_t;
typedef struct cbtree_t cbtree_t;
typedef struct cbnode_t cbnode_t;

typedef cbtree_bool_t (*cbtree_equal_fn)(const void *__restrict__ value1, const void *__restrict__ value2, unsigned long keysize);

extern cbtree_t *cbt_init(int datasize);
extern cbnode_t *cbt_insert(cbtree_t *tree, const void *value);
extern void cbt_delete(cbtree_t *tree, const void *value);
extern cbnode_t *cbt_search(cbtree_t *tree, const void *data);
extern void *cbt_node_data(cbnode_t *node);

#ifdef __cplusplus
    }
#endif

#endif//__CBINARY_TREE_H