#ifndef __CG_FREELIST_H
#define __CG_FREELIST_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct cg_node_t {
  struct cg_node_t *next;
  struct cg_node_t *prev;
  void *alloc;
  int size;
  bool in_use;
} cg_node_t;

typedef void *(*cg_freelist_alloc_fn)(size_t size);
typedef void *(*cg_freelist_free_fn)(void *data);

typedef struct cg_freelist_t {
  cg_node_t *root;
  cg_freelist_alloc_fn alloc_fn;
  cg_freelist_free_fn free_fn;
} cg_freelist_t;

cg_node_t *cg_freelist_mknode(const cg_freelist_t *list, int size, cg_node_t *next, cg_node_t *prev) {
  cg_node_t *node = luna_malloc(sizeof(cg_node_t));
  node->next      = next;
  node->prev      = prev;
  node->alloc     = list->alloc_fn(size);
  node->size      = size;
  node->in_use    = 0;
  return node;
}

void cg_freelist_init(int init_size, cg_freelist_alloc_fn alloc_fn, cg_freelist_free_fn free_fn, cg_freelist_t *list) {
  list->alloc_fn = alloc_fn;
  list->free_fn  = free_fn;
  list->root     = cg_freelist_mknode(list, init_size, NULL, NULL);
}

void *cg_freelist_alloc(cg_freelist_t *list, int size) {
  cg_node_t *node = list->root;

  while (node) {
    if (node->in_use) {
      node = node->next;
      continue;
    }

    if (node == list->root && list->root->in_use && !list->root->next) {
      if (list->root->size < size) {
        luna_free(list->root->alloc);
        list->root->alloc = luna_malloc(size);
        list->root->size  = size;
      }
      list->root->in_use = 1;
      return list->root->alloc;
    }

    if (node->size == size) {
      node->in_use = 1;
      return node->alloc;
    } else if (node->size > size) {
      // this avoids creating two nodes and instead just reuses the old node
      cg_node_t *new_node = cg_freelist_mknode(list, node->size - size, node->next, node);
      node->next          = new_node;
      node->size -= size;
      new_node->in_use = 0;
      new_node->alloc  = (char *)node->alloc + size;
      node->in_use     = 1;
      return node->alloc;
    }
    node = node->next;
  }

  return NULL;
}

cg_node_t *cg_freelist_expand(cg_freelist_t *list, int expand_by) {
  cg_node_t *node = list->root;
  while (node->next) {
    node = node->next;
  }

  cg_node_t *new_node = cg_freelist_mknode(list, expand_by, NULL, node);
  node->next          = new_node;
  return new_node;
}

#endif //__CG_FREELIST_H