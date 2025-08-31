#ifndef OPIUM_DLQUEUE_INCLUDE_H
#define OPIUM_DLQUEUE_INCLUDE_H

#include "core/opium_list.h"
#include "core/opium_log.h"
#include <stdint.h>

typedef struct opium_dlqueue_s opium_dlqueue_t;

struct opium_dlqueue_s {
   struct opium_list_head head;
   opium_log_t *log;
};

typedef struct opium_dlqueue_node_s opium_dlqueue_node_t;

typedef uintptr_t opium_rbt_key_t;

struct opium_dlqueue_node_s {
   struct opium_list_head head;
   opium_rbt_key_t key;
   void *data;
};

void opium_dlqueue_init(opium_dlqueue_t *queue);
int opium_dlqueue_is_empty(opium_dlqueue_t *queue);

void opium_dlqueue_push(opium_dlqueue_t *queue, opium_dlqueue_node_t *node);
opium_dlqueue_node_t *opium_dlqueue_pop(opium_dlqueue_t *queue);

opium_dlqueue_node_t *opium_dlqueue_first(opium_dlqueue_t *queue);

void opium_dlqueue_debug(opium_dlqueue_t *queue);

#endif /* OPIUM_DLQUEUE_INCLUDE_H */
