#include "pool.h"
#include "utils.h"
#include "queue.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (((a) > (b)) ? (b) : (a))

int onion_ringbuff_size(onion_ringbuff_t *ringbuff) {
   return ringbuff->capacity - 1;
}

uint8_t *onion_ringbuff_end(onion_ringbuff_t *ringbuff) {
   return ringbuff->data + onion_ringbuff_size(ringbuff); 
}

int onion_ringbuff_bytes_busy(onion_ringbuff_t *ringbuff) {
   return (ringbuff->head + onion_ringbuff_size(ringbuff) - ringbuff->tail) % onion_ringbuff_size(ringbuff);
}

size_t onion_ringbuff_bytes_free(onion_ringbuff_t *ringbuff) {
   if (ringbuff->head >= ringbuff->tail) {
      return onion_ringbuff_size(ringbuff) - (ringbuff->head - ringbuff->tail);
   } else {
      return ringbuff->tail - ringbuff->head - 1;
   }
}

int onion_ringbuff_isEmpty(onion_ringbuff_t *ringbuff) {
   return onion_ringbuff_bytes_busy(ringbuff) == 0;
}

int onion_ringbuff_isFull(onion_ringbuff_t *ringbuff) {
   return onion_ringbuff_bytes_free(ringbuff) == 0; 
}

void onion_ringbuff_reset(onion_ringbuff_t *ringbuff) {
   ringbuff->head = ringbuff->tail = ringbuff->data;
}

size_t onion_ringbuff_read(onion_ringbuff_t *ringbuff) {
   if (onion_ringbuff_bytes_busy(ringbuff) < 4) {
      DEBUG_FUNC("Is free!\n");
      return 0;
   }

   uint32_t size;
   int tail_offset = ringbuff->tail - ringbuff->data;

   if (tail_offset + 4 <= onion_ringbuff_size(ringbuff)) {
      memcpy(&size, ringbuff->tail, 4);
   } else {
      int first_part = onion_ringbuff_size(ringbuff) - tail_offset;
      memcpy(&size, ringbuff->tail, first_part);
      memcpy((uint8_t *)&size + first_part, ringbuff->data, 4 - first_part);
   }

   return size;
}

int onion_ringbuff_extract(onion_ringbuff_t *ringbuff, void *src, size_t size) {
   int read = onion_ringbuff_read(ringbuff);

   if (read == 0) {
      DEBUG_FUNC("No read\n");
      return 0;
   }

   if ((int)size < read) {
      DEBUG_FUNC("Not enough space in src buff\n!");
      return 0;
   } 

   uint8_t *current_data = ringbuff->tail + 4;
   int data_offset = current_data - ringbuff->data;

   if (data_offset + read <= onion_ringbuff_size(ringbuff)) {
      memcpy(src, current_data, read);
      ringbuff->tail = ringbuff->tail + read + 4;

      if (ringbuff->tail > onion_ringbuff_end(ringbuff)) {
         ringbuff->tail = ringbuff->data + (ringbuff->tail - onion_ringbuff_end(ringbuff));
      }
   } else {
      int first_part = onion_ringbuff_size(ringbuff) - data_offset;
      int second_part = read - first_part;
      memcpy(src, current_data, first_part);
      memcpy((uint8_t *)src + first_part, ringbuff->data, second_part);
      ringbuff->tail = ringbuff->data + second_part + 4;
   }

   return read;
}

uint8_t *onion_ringbuff_write(onion_ringbuff_t *ringbuff, void *data, size_t size) {
   const uint8_t *buff_dst = data;
   const uint8_t *buff_end = onion_ringbuff_end(ringbuff); 
   size_t free_space = onion_ringbuff_bytes_free(ringbuff);
   size_t count = MIN(size, free_space);
   size_t read = 0;

   if (free_space < size) {
      DEBUG_FUNC("Not enough space!\n");
      return NULL;
   }

   while (size != read) {
      size_t space_to_end = buff_end - ringbuff->head;
      size_t throw = MIN(space_to_end, count - read);
      memcpy(ringbuff->head, buff_dst + read, throw);
      ringbuff->head = ringbuff->head + throw;
      read = read + throw;

      if (ringbuff->head == buff_end) {
         ringbuff->head = ringbuff->data;
      }
   }

   return ringbuff->head;
}

uint8_t *onion_ringbuff_dmemcpy(onion_ringbuff_t *ringbuff, void *data, size_t size) {
   uint32_t header = (uint32_t)size;

   if (onion_ringbuff_isFull(ringbuff)) {
      DEBUG_FUNC("Ring buffer is fullm.\n");
      return NULL;
   }

   if (onion_ringbuff_bytes_free(ringbuff) < size + sizeof(header)) {
      DEBUG_FUNC("Ring buffer spase isn`t enough.\n");
      return NULL;
   }

   if (onion_ringbuff_write(ringbuff, &header, sizeof(int)) == NULL) {
      DEBUG_FUNC("Size write failed\n");
      return NULL;
   }

   if (onion_ringbuff_write(ringbuff, data, size) == NULL) {
      DEBUG_FUNC("Data write failed\n");
      return NULL;
   }

   return ringbuff->head;
}

uint8_t *onion_ringbuff_omemcpy(onion_ringbuff_t *ringbuff, void *data, size_t size) {
   return 0;
}

uint8_t *onion_ringbuff_memcpy(onion_ringbuff_t *ringbuff, void *data, size_t size) {
   return ringbuff->overflow ? onion_ringbuff_omemcpy(ringbuff, data, size) : onion_ringbuff_dmemcpy(ringbuff, data, size);
}

int onion_ringbuff_init(onion_ringbuff_t **ptr, size_t capacity, bool overflow) {
   int ret;
   onion_ringbuff_t *ringbuff = malloc(sizeof(onion_ringbuff_t)); 

   if (!ringbuff) {
      DEBUG_FUNC("Queue initialization failed!\n");
      goto unssuccessfull;
   }

   ringbuff->capacity = capacity + 1;
   ringbuff->overflow = overflow;

   ringbuff->data = malloc(ringbuff->capacity);
   if (!ringbuff->data) {
      DEBUG_FUNC("Fds initialization failed!\n");
      goto please_free;
   }

   onion_ringbuff_reset(ringbuff);
   DEBUG_FUNC("head: %p \n", ringbuff->head);
   *ptr = ringbuff;
   return 0;

please_free:
   onion_ringbuff_exit(ringbuff);
unssuccessfull:
   return -1;
}

void onion_ringbuff_exit(onion_ringbuff_t *ringbuff) {
   if (ringbuff->data) {
      free(ringbuff->data);
      ringbuff->data = NULL;
   }

   free(ringbuff);
}
