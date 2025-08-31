#ifndef opium_CONFIG_INCLUDE_H
#define opium_CONFIG_INCLUDE_H

#include "core/opium_log.h"
#include "core/opium_pool.h"

typedef enum {
    opium_CONF_INT,
    opium_CONF_SIZE,
    opium_CONF_PTR,
    opium_CONF_FUNC,
} opium_conf_type_t;

typedef struct {
   const char *key;
   opium_conf_type_t type;
   union {
      int number;
      size_t size;
      void *ptr;
      void (*func)(void);
   } value;
} opium_conf_entry_t;

typedef struct {
   char *name;

   opium_conf_entry_t *entries;
   size_t count;

   opium_log_t *log;
} opium_conf_t;

#endif /* opium_CONFIG_INCLUDE_H */
