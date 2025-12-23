#ifndef OPIUM_RBTREE_INCLUDE_H
#define OPIUM_RBTREE_INCLUDE_H

#include "core/opium_core.h"

typedef uintptr_t opium_rbt_key_t;

typedef struct opium_rbt_node_s opium_rbt_node_t;
struct opium_rbt_node_s {
   opium_rbt_node_t *parent;
   opium_rbt_node_t *right;
   opium_rbt_node_t *left;

   opium_rbt_key_t key;
   void *data;
   u_char color;
};

struct opium_rbt_s {
   opium_rbt_node_t *head;
   opium_rbt_node_t *sentinel;

   opium_slab_t slab;

   opium_log_t *log;
};

#define opium_rbt_head(rbtree) (rbtree->head)
#define opium_rbt_root(node) (node->parent)

#define opium_rbt_black(node) ((node)->color = 0)
#define opium_rbt_red(node) ((node)->color = 1)
#define opium_rbt_is_red(node) ((node)->color)
#define opium_rbt_is_black(node) (!opium_rbt_is_red(node))
#define opium_rbt_copy_color(node1, node2) ((node1)->color = (node2)->color)

#define opium_rbt_sentinel_init(sentinel) \
   opium_rbt_black(sentinel);             \
   sentinel->parent = sentinel;           \
   sentinel->right = sentinel;            \
   sentinel->left = sentinel;

int opium_rbt_init(opium_rbt_t *rbt, opium_log_t *log);
void opium_rbt_exit(opium_rbt_t *rbt);

opium_rbt_node_t *opium_rbt_insert(opium_rbt_t *rbt, opium_rbt_key_t key, void *data);
void opium_rbt_delete(opium_rbt_t *rbt, opium_rbt_key_t key);

static inline void opium_rbt_insert_data(opium_rbt_node_t *node, void *data) {
    if (!node) return;
    node->data = data;
}

#endif /* OPIUM_RBTREE_INCLUDE_H */

