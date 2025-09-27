#ifndef OPIUM_LOG_INCLUDE_H
#define OPIUM_LOG_INCLUDE_H

#include "core/opium_core.h"

typedef struct opium_ialloc_s opium_ialloc_t;

struct opium_ialloc_s {
   void **elts;
   size_t nelts;

   opium_uint_t *free_list;

 opium_arena_t *arena;  

};

#endif /* OPIUM_LOG_INCLUDE_H */
