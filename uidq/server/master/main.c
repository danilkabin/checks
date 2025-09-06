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

#include "core/opium_slab.h"
#include "core/opium_string.h"
#include "udop/opium_avl.h"
#include "udop/opium_bst.h"

int main() {
   opium_log_conf_t log_conf = {.debug = NULL, .warn = NULL, .err = NULL};
   opium_log_t *log = opium_log_create(&log_conf);

   opium_slab_conf_t conf = {.count = 4}; 
   opium_slab_t *slab = opium_slab_create(&conf, log);
   if (!slab) return -1;

   opium_rbt_t *rbt = opium_rbt_init(4, log);
   if (!rbt) {
      printf("RBT init failed\n");
      return -1;
   }

   printf("=== Test 1: Insert duplicate keys ===\n");
   opium_rbt_insert(rbt, 10);
   opium_rbt_insert(rbt, 10); // duplicate
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   printf("=== Test 2: Delete non-existent key ===\n");
   opium_rbt_delete(rbt, 999);
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   printf("=== Test 3: Insert more nodes than pool capacity ===\n");
   opium_rbt_insert(rbt, 20);
   opium_rbt_insert(rbt, 30);
   opium_rbt_node_t *extra = opium_rbt_insert(rbt, 40);
   if (!extra) printf("[ERROR]: Pool limit reached, insert failed\n");
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   printf("=== Test 4: Delete root repeatedly ===\n");
   opium_rbt_delete(rbt, 10);
   opium_rbt_delete(rbt, 20);
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   printf("=== Test 5: Insert negative and zero keys ===\n");
   opium_rbt_insert(rbt, 0);
   opium_rbt_insert(rbt, -5);
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   printf("=== Test 6: Random stress insert/delete ===\n");
   srand(time(NULL));
   for (int i = 0; i < 20; i++) {
      int key = (rand() % 100) - 50; // -50..49
      opium_rbt_node_t *node = opium_rbt_insert(rbt, key);
      if (!node) printf("Insert %d failed (pool limit or duplicate)\n", key);
   }

   for (int i = 0; i < 20; i++) {
      int key = (rand() % 100) - 50;
      opium_rbt_delete(rbt, key);
   }

   printf("Tree after random stress operations:\n");
   opium_rbt_debug(rbt, rbt->sentinel, rbt->head);
   printf("\n");

   opium_rbt_exit(rbt);

   return 0;
}
