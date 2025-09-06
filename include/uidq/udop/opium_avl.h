#ifndef OPIUM_AVL_INCLUDE_H
#define OPIUM_AVL_INCLUDE_H

#include "core/opium_config.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stdint.h>
#include <sys/types.h>

typedef struct opium_avl_s opium_avl_t;
struct opium_avl_s {
   opium_avl_t *right; 
   opium_avl_t *left; 

   int key;
   u_char height;
};

opium_avl_t *opium_avl_init(int key);

opium_avl_t *opium_avl_insert(opium_avl_t *node, int key);

void opium_avl_debug(opium_avl_t *node);

#endif /* OPIUM_AVL_INCLUDE_H */
