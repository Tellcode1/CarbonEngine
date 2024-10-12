#include <stdlib.h>
#include "../../include/defines.h"
#include "../../include/credblacktree.h"

#define CRB_COL_R 1
#define CRB_COL_B 0

typedef struct crbnode_t {
    void *data;
    crbnode_t *l, *r, *p; // parent
    unsigned char col;
} crbnode_t;

typedef struct crbtree_t {
    crbnode_t *root;
    int datasize;
} crbtree_t;

crbtree_t *crbt_init(int datasize)
{
    crbtree_t *tree = malloc(sizeof(struct crbtree_t));
    tree->datasize = datasize;
    return tree;
}

crbnode_t *crbt_mknode(const crbtree_t *tree, const void *data) {
    crbnode_t *node = malloc(sizeof(struct crbnode_t));
    node->l = NULL;
    node->r = NULL;
    node->p = NULL;
    node->col = 0;
    node->data = malloc(tree->datasize);
    memcpy(node->data, data, tree->datasize);
}

crbnode_t *crb_std_binary_tree_insert(crbtree_t *tree, const void *value) {
    crbnode_t *node = crbt_mknode( tree->datasize, value );
    // if there is no root, then just make this node the root.
    if (tree->root == NULL) {
        tree->root = node;
        return node;
    }

    // lvl order traversal

    int queue_size = 64;
    crbnode_t **queue = malloc(queue_size * sizeof(crbnode_t *));
    int front = -1, rear = -1;

    queue[++rear] = tree->root;
    while (front != rear) {
        crbnode_t *temp = queue[++front];
        if (rear >= queue_size - 1) {
            queue_size *= 2;
            queue = realloc(queue, queue_size * sizeof(crbnode_t *));
            cassert(queue != NULL);
        }
        if (temp->l == NULL) {
            temp->l = node;
            break;
        } else {
            queue[++rear] = temp->l;
        }
        if (temp->r == NULL) {
            temp->r = node;
            break;
        } else {
            queue[++rear] = temp->r;
        }
    }

    free(queue);
    return node;
}

crbnode_t *crbt_insert(crbtree_t *tree, const void *value)
{
    crbnode_t *node = crb_std_binary_tree_insert( tree, value );
    if (tree->root == node) {
        node->col = CRB_COL_B;
    }
    return node;
}