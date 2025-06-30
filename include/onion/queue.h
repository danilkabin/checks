#ifndef ONION_QUEUE_H
#define ONION_QUEUE_H

#include "pool.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
   uint8_t *data;
   uint8_t *head, *tail;
   size_t capacity;
   bool overflow;
} onion_ringbuff_t;


int onion_ringbuff_size(onion_ringbuff_t *ringbuff);
uint8_t *onion_ringbuff_end(onion_ringbuff_t *ringbuff);
int onion_ringbuff_bytes_busy(onion_ringbuff_t *ringbuff);
size_t onion_ringbuff_bytes_free(onion_ringbuff_t *ringbuff);
int onion_ringbuff_isEmpty(onion_ringbuff_t *ringbuff);
int onion_ringbuff_isFull(onion_ringbuff_t *ringbuff);

   int onion_ringbuff_init(onion_ringbuff_t **, size_t, bool);
void onion_ringbuff_exit(onion_ringbuff_t *);

uint8_t *onion_ringbuff_memcpy(onion_ringbuff_t *ringbuff, void *data, size_t size);
int onion_ringbuff_extract(onion_ringbuff_t *ringbuff, void *src, size_t size);

#endif
