#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_config.h"
#include "core/opium_core.h"
#include "core/opium_dlqueue.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

int main() {
   opium_log_conf_t log_conf = {.debug = NULL, .warn = NULL, .err = NULL};
   opium_log_t *log = opium_log_create(&log_conf);

   opium_rbt_node_t nill;
   nill.parent = NULL;
   nill.right = NULL;
   nill.left = NULL;
   opium_rbt_ins_func_t ins = opium_rbt_insert1;

   opium_dlqueue_t *queue = opium_calloc(sizeof(opium_dlqueue_t), NULL);
   if (!queue) {
      return -1;
   }
   opium_dlqueue_init(queue);

   for (int index = 0; index < 10; index++) {
      opium_dlqueue_node_t *node = opium_calloc(sizeof(opium_dlqueue_node_t), NULL);
      node->key = index;
      opium_dlqueue_push(queue, node);
   }

   opium_dlqueue_debug(queue);

   for (int index = 0; index < 13; index++) {
      opium_dlqueue_pop(queue);
      opium_dlqueue_debug(queue);
   }

   /*  opium_rbt_t *rbtree = opium_rbt_create(&nill, ins, log);
       if (!rbtree) {
       opium_err(log, "Noooo!\n"); 
       return -1;
       }

       for (int index = 0; index < 30; index++) {
       opium_rbt_node_t *node = opium_calloc(sizeof(opium_rbt_node_t), NULL);
       node->data = index;
       opium_rbt_insert(rbtree, node);
       }


       opium_rbt_debug(rbtree->head, rbtree->nill, 0);
       */
   opium_debug(log, "HEllo!\n"); 
}
