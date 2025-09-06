#ifndef OPIUM_HASHFUNC_INCLUDE_H
#define OPIUM_HASHFUNC_INCLUDE_H

#include <stddef.h>
#include <stdint.h>
   
   uint64_t 
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
   
   uint64_t 
opium_hash_sdbm(void *raw_key, size_t key_size) 
{
   if (!raw_key || key_size == 0) {
      return 0;
   }

   uint64_t hash = 0;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash = key[i] + (hash << 6) + (hash << 16) - hash;
   }

   return hash;
}

   uint64_t
opium_hash_fnv1a( void *raw_key, size_t key_size) 
{
   if (!raw_key || key_size == 0) {
      return 0;
   }

   uint64_t hash = 14695981039346656037ULL;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash ^= key[i];
      hash *= 1099511628211ULL;
   }

   return hash;
}

   uint64_t
opium_hash_jenkins( void *raw_key, size_t key_size) 
{
   if (!raw_key || key_size == 0) {
      return 0;
   }

   uint64_t hash = 0;
   unsigned char *key = raw_key;

   for (size_t i = 0; i < key_size; i++) {
      hash += key[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }
   hash += (hash << 3);
   hash ^= (hash >> 11);
   hash += (hash << 15);

   return hash;
}

   uint64_t
opium_hash_murmur3( void *raw_key, size_t key_size)
{
   if (!raw_key || key_size == 0) {
      return 0;
   }

   uint64_t seed = 0xc70f6907UL;
   uint64_t m = 0xc6a4a7935bd1e995ULL;
   int r = 47;

   uint64_t hash = seed ^ (key_size * m);
   uint64_t *data = ( uint64_t *)raw_key;
   uint64_t *end = data + (key_size / 8);

   while (data != end) {
      uint64_t k = *data++;
      k *= m; k ^= k >> r; k *= m;
      hash ^= k;
      hash *= m;
   }

   unsigned char *data2 = ( unsigned char *)data;
   switch (key_size & 7) {
      case 7: hash ^= (uint64_t)data2[6] << 48;
      case 6: hash ^= (uint64_t)data2[5] << 40;
      case 5: hash ^= (uint64_t)data2[4] << 32;
      case 4: hash ^= (uint64_t)data2[3] << 24;
      case 3: hash ^= (uint64_t)data2[2] << 16;
      case 2: hash ^= (uint64_t)data2[1] << 8;
      case 1: hash ^= (uint64_t)data2[0]; hash *= m;
   }

   hash ^= hash >> r;
   hash *= m;
   hash ^= hash >> r;

   return hash;
}

#endif /* OPIUM_HASH_INCLUDE_H */
