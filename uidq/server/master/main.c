#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_config.h"
#include "core/opium_core.h"
#include "core/opium_dlqueue.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_radixtree.h"
#include "core/opium_rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdint.h>

#include "udop/opium_bst.h"

int main() {
   opium_log_conf_t log_conf = {.debug = NULL, .warn = NULL, .err = NULL};
   opium_log_t *log = opium_log_create(&log_conf);

   opium_bst_node_t *node = opium_bst_node_init(100); 
   if (!node) {
      return -1;
   }

   for (int index = 0; index < 200; index = index + 20) {
      opium_bst_node_insert(node, index);
   }

   opium_bst_node_debug(node);

   opium_bst_node_remove(node, 40);
   opium_bst_node_remove(node, 160);
   opium_bst_node_remove(node, 134);

   int height = opium_bst_node_height(node);
   printf("height: %d\n", height);

   opium_bst_node_debug(node);

   /*opium_rbt_node_t *nill = calloc(1, sizeof(opium_rbt_node_t));
     opium_rbt_black(nill);
     nill->left = nill->right = nill->parent = nill;
     opium_rbt_ins_func_t ins = opium_rbt_insert1;

     opium_rbt_t *rbtree = opium_rbt_create(nill, ins, log);
     if (!rbtree) {
     opium_err(log, "Noooo!\n"); 
     return -1;
     }

     for (int index = 0; index < 5; index++) {
     opium_rbt_node_t *node = opium_calloc(sizeof(opium_rbt_node_t), NULL);
     node->data = index;
     opium_rbt_insert(rbtree, node);
     }


     opium_rbt_debug(rbtree->head, rbtree->nill, 0);
     */
   opium_debug(log, "HEllo!\n"); 
}
