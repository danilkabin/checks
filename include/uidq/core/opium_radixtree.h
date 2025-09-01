#ifndef OPIUM_RADIXTREE_INCLUDE_H
#define OPIUM_RADIXTREE_INCLUDE_H

#include "core/opium_config.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stdint.h>
#include <sys/types.h>

/* I did this by referring to Salvatore Sanfilippo's implementation. */ 

typedef u_char opium_radix_data_t;

typedef struct opium_radix_node_s opium_radix_node_t;
struct opium_radix_node_s {
   uint32_t iskey:1;   /* Does this node contain a key? */
   uint32_t isnull:1;  /* Is null yes */
   uint32_t iscompr:1; /* Is compressed */
   uint32_t size:29;   /* Number of children, or compressed string len. */

   opium_radix_data_t data[];
};

typedef struct opium_radix_s opium_radix_t;
struct opium_radix_s {
   int initialized;

   opium_radix_node_t *head; 
   size_t ndnm; /* Node count */
   size_t elnm; /* Element count */

   opium_log_t *log;
};

/* We add 4 to the node size because the node has a four bytes header */
#define opium_radix_padding(size) ((sizeof(void*) - (size + 4) % sizeof(void*)) & (sizeof(void*) - 1))

opium_radix_t *opium_radix_init(opium_log_t *log);

int opium_radix_insert(opium_radix_t *radix, u_char *s, size_t len, void *data, void **old);

#endif /* OPIUM_RADIXTREE_INCLUDE_H */
