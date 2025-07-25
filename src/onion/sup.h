#ifndef ONION_SUP_H
#define ONION_SUP_H

#include <sys/types.h>

typedef struct {
   u_int64_t *mask;
   size_t size;
   size_t conv_size;
   size_t size_per_frame;
} onion_bitmask;


void onion_set_bit(onion_bitmask *, size_t);
void onion_clear_bit(onion_bitmask *, size_t);

int onion_ffb(onion_bitmask *, int, int);
int onion_find_bit(onion_bitmask *, int);
int onion_find_grab_bit(onion_bitmask *, int, int);

int onion_bitmask_add(onion_bitmask *, size_t );
void onion_bitmask_del(onion_bitmask *, int , size_t );
int onion_bitmask_init(onion_bitmask **, size_t , size_t);
void onion_bitmask_exit(onion_bitmask *);

static inline size_t onion_round_size_pow2(size_t size) {
   if (size <= 0) {
      return 1;
   }
   size_t multiply = 1;

   while (size > multiply) {
      multiply <<= 1;
   }

   return multiply;
}

#endif
