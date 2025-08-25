#ifndef UIDQ_CONFIG_INCLUDE_H
#define UIDQ_CONFIG_INCLUDE_H

#include "core/uidq_log.h"
#include "core/uidq_pool.h"

typedef enum {
    UIDQ_CONF_INT,
    UIDQ_CONF_SIZE,
    UIDQ_CONF_PTR,
    UIDQ_CONF_FUNC,
} uidq_conf_type_t;

typedef struct {
   const char *key;
   uidq_conf_type_t type;
   union {
      int number;
      size_t size;
      void *ptr;
      void (*func)(void);
   } value;
} uidq_conf_entry_t;

typedef struct {
   char *name;

   uidq_conf_entry_t *entries;
   size_t count;

   uidq_log_t *log;
} uidq_conf_t;

#endif /* UIDQ_CONFIG_INCLUDE_H */
