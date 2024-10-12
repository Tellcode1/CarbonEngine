#include <stdlib.h>
#include <string.h>
#include "../../include/cbinarytree.h"
#include "../../include/defines.h"

typedef struct cbnode_t {
    void *data;
    cbnode_t *l, *r;
} cbnode_t;

typedef struct cbtree_t {
    int datasize;
    cbnode_t *root;
    cbtree_equal_fn equal_fn;
} cbtree_t;

cbnode_t *mknode(int datasize, const void *data) {
    cbnode_t *node = malloc(sizeof(cbnode_t));
    cassert(node != NULL);

    node->l = NULL;
    node->r = NULL;
    node->data = malloc(datasize);
    memcpy(node->data, data, datasize);

    return node;
}

cbnode_t *cbt_insert(cbtree_t *tree, const void *value) {
    cbnode_t *node = mknode( tree->datasize, value );
    // if there is no root, then just make this node the root.
    if (tree->root == NULL) {
        tree->root = node;
        return node;
    }

    // lvl order traversal

    int queue_size = 64;
    cbnode_t **queue = malloc(queue_size * sizeof(cbnode_t *));
    int front = -1, rear = -1;

    queue[++rear] = tree->root;
    while (front != rear) {
        cbnode_t *temp = queue[++front];
        if (rear >= queue_size - 1) {
            queue_size *= 2;
            queue = realloc(queue, queue_size * sizeof(cbnode_t *));
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

void cbt_delete(cbtree_t *tree, const void *value) {
    if (tree->root == NULL) {
        return;
    }

    int queue_size = 64;
    cbnode_t **queue = malloc(queue_size * sizeof(cbnode_t *));
    if (queue == NULL) {
        return;
    }

    int front = -1, rear = -1;
    queue[++rear] = tree->root;
    cbnode_t *target = NULL;
    cbnode_t *last = NULL;

    while (front != rear) {
        last = queue[++front];
        if (rear >= queue_size - 1) {
            queue_size *= 2;
            queue = realloc(queue, queue_size * sizeof(cbnode_t *));
            if (queue == NULL) {
                return;
            }
        }

        if (tree->equal_fn(last->data, value, tree->datasize)) {
            target = last;
        }
        if (last->l != NULL) {
            queue[++rear] = last->l;
        }
        if (last->r != NULL) {
            queue[++rear] = last->r;
        }
    }

    // What? Am I supposed to explain what this code does????
    if (target != NULL) {
        memcpy(target->data, last->data, tree->datasize);

        cbnode_t *parent = NULL;
        front = -1;
        rear = -1;
        queue[++rear] = tree->root;

        while (front != rear) {
            parent = queue[++front];
            if (parent->l == last) {
                parent->l = NULL;
                free(last);
                break;
            }
            if (parent->r == last) {
                parent->r = NULL;
                free(last);
                break;
            }
            if (parent->l != NULL) {
                queue[++rear] = parent->l;
            }
            if (parent->r != NULL) {
                queue[++rear] = parent->r;
            }
        }
    }

    free(queue);
}

void *cbt_node_data(cbnode_t *node)
{
    return node->data;
}

cbnode_t *cbt_search(cbtree_t *tree, const void *data)
{
    if (tree->root == NULL) {
        return NULL;
    }

    int queue_size = 64;
    cbnode_t **queue = malloc(queue_size * sizeof(cbnode_t *));

    int front = -1, rear = -1;
    queue[++rear] = tree->root;
    while (front != rear) {
        cbnode_t *temp = queue[++front];
        if (rear >= queue_size - 1) {
            queue_size *= 2;
            queue = realloc(queue, queue_size * sizeof(cbnode_t *));
            cassert(queue != NULL);
        }
        if (tree->equal_fn(temp->data, data, tree->datasize)) {
            return temp;
        }
        if (temp->l != NULL) {
            queue[++rear] = temp->l;
        }
        if (temp->r != NULL) {
            queue[++rear] = temp->r;
        }
    }

    free(queue);
    return NULL;
}

// v1 will never be equal to v2 because the btree makes a copy of the data
static inline cbtree_bool_t compmem(const void *v1, const void *v2, unsigned long l) {
    return (memcmp(v1, v2, l) == 0);
}

cbtree_t *cbt_init(int datasize) {
    cbtree_t *tree = malloc(sizeof(cbtree_t));
    cassert(tree != NULL);
    tree->datasize = datasize;
    tree->equal_fn = compmem;
    tree->root = NULL;
    return tree;
}