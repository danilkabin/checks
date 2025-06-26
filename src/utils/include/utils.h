#ifndef UTILS_H
#define UTILS_H

#include "pool.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static inline size_t round_size_pow2(size_t size) {
   if (size <= 0) {
      return 1;
   }
   size_t multiply = 1;

   while (size > multiply) {
      multiply <<= 1;
   }

   return multiply;
}

static inline int ffs_8(uint8_t byte) {
   if (byte == 0) {
      return -1;
   };
   return __builtin_ffs(byte) - 1;
}

static inline int ffz_8(uint8_t byte) {
   return ffs_8(~byte);
}

static inline int ffb(uint8_t *bitmap, size_t count) {
   for (size_t byte = 0; byte < (count + 7) / 8; ++byte) {
      if (bitmap[byte] != 0xFF) {
         for (int bit = 0; bit < 8; ++bit) {
            if (!(bitmap[byte] & (1 << bit))) {
               int index = byte * 8 + bit;
               if ((size_t)index >= count) {
                  break;
               }
               if ((size_t)index < count) {
                  return index;
               }
            }
         }
      }
   }
   return -1;
}

static inline void printBitmap(uint8_t *bitmap, size_t bytes) {
   for (size_t byte = 0; byte < bytes; byte++) {
      for (int bit = 7; bit >= 0; bit--) {
         printf("%d", (bitmap[byte] >> bit) & 1);
      }
      printf(" ");
   }
   printf("\n");
}

static inline void set_bit(uint8_t *bitmap, size_t offset) {
   bitmap[offset / 8] |= (1 << offset % 8);
}

static inline void clear_bit(uint8_t *bitmap, size_t offset) {
   bitmap[offset / 8] &= ~(1 << offset % 8);
}

#define DEBUG_INFO(fmt, ...) do { \
   printf(fmt, ##__VA_ARGS__); \
} while(0)

#define DEBUG_FUNC(fmt, ...) do { \
   printf("\n%s: " fmt, __func__, ##__VA_ARGS__); \
} while(0)

#define DEBUG_ERR(fmt, ...) do { \
   printf("\n%s: err:  %s  " fmt, __func__, strerror(errno), ##__VA_ARGS__); \
} while(0)

#define CHECK_NULL_RETURN(ptr, msg) do { \
   if (!(ptr)) { \
      DEBUG_INFO(msg); \
      return -1; \
   } \
} while(0)

#define CHECK_ZERO_RETURN(num, msg) do { \
   if ((num) == 0) { \
      DEBUG_INFO(msg); \
      return NULL; \
   } \
} while(0)

#define CHECK_MY_THREAD(fail, thread, attr, func, arg) do { \
   int ret = pthread_create(thread.flow, attr, func, arg); \
   if (ret != 0) { \
      fail; \
   } \
   mutex_init(thread.mutex, NULL); \
   DEBUG_FUNC("thread was inited\n"); \
} while(0)

#endif
