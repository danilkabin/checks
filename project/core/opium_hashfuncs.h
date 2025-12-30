#ifndef OPIUM_HASHFUNC_INCLUDE_H
#define OPIUM_HASHFUNC_INCLUDE_H

#include "core/opium_core.h"

   static inline opium_u64_t 
opium_hash_djb2(void *raw_key, size_t key_size)
{
   if (!raw_key || key_size == 0) {
      return 0;
   }

   size_t hash = 5381;
   char *key = raw_key;

   for (size_t byte = 0; byte < key_size; ++byte) {
      hash = ((hash << 5) + hash) ^ key[byte];
   }

   return hash;
}
   
#endif /* OPIUM_HASH_INCLUDE_H */
