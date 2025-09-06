#ifndef OPIUM_RBTREE_INCLUDE_H
#define OPIUM_RBTREE_INCLUDE_H

#include "core/opium_config.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stdint.h>
#include <sys/types.h>

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

typedef struct opium_rbt_s opium_rbt_t;
struct opium_rbt_s {
   opium_rbt_node_t *head;
   opium_rbt_node_t *sentinel;
   
   opium_pool_t *pool;

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

int opium_rbt_is_valid(opium_rbt_t *rbt);
opium_rbt_t *opium_rbt_init(size_t capacity, opium_log_t *log);
void opium_rbt_exit(opium_rbt_t *rbtree);

opium_rbt_node_t *opium_rbt_node_init(opium_rbt_t *rbt, opium_rbt_node_t *parent, opium_rbt_key_t key);

opium_rbt_node_t *opium_rbt_insert(opium_rbt_t *rbt, opium_rbt_key_t key);
void opium_rbt_delete(opium_rbt_t *rbt, opium_rbt_key_t key);

void opium_rbt_node_balance(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *node);

opium_rbt_node_t *opium_rbt_left_rotate(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *node);
opium_rbt_node_t *opium_rbt_right_rotate(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *node);

void opium_rbt_debug(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *node);

#endif /* OPIUM_RBTREE_INCLUDE_H */
