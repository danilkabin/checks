#ifndef OPIUM_ARRAY_INCLUDE_H
#define OPIUM_ARRAY_INCLUDE_H

#include "core/opium_config.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stdint.h>
#include <sys/types.h>

typedef struct opium_array_s opium_array_t;
struct opium_array_s {
   int initialized;

   opium_pool_t *pool;

   opium_log_t *log;
};

#endif /* OPIUM_ARRAY_INCLUDE_H */
