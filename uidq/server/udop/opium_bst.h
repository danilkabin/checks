#ifndef OPIUM_BST_INCLUDE_H
#define OPIUM_BST_INCLUDE_H

#include "core/opium_config.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stdint.h>
#include <sys/types.h>

typedef struct opium_bst_node_s opium_bst_node_t;
struct opium_bst_node_s {
   opium_bst_node_t *right; 
   opium_bst_node_t *left; 

   u_char data;
};
opium_bst_node_t *opium_bst_node_init(int data);

opium_bst_node_t *opium_bst_node_find(opium_bst_node_t *root, int data);

opium_bst_node_t *opium_bst_node_insert(opium_bst_node_t *root, int data);
opium_bst_node_t *opium_bst_node_remove(opium_bst_node_t *root, int data);

int opium_bst_node_height(opium_bst_node_t *root);

void opium_bst_node_debug(opium_bst_node_t *node);

#endif /* OPIUM_BST_INCLUDE_H */
