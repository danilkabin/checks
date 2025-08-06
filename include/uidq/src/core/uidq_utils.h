#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"

#define UIDQ_DEBUG_ENABLED 4308
#define UIDQ_DEBUG_DISABLED 4309

#define UIDQ_MIN(a,b) ((a) < (b) ? (a) : (b))

#define DEBUG_INFO(fmt, ...) do { \
    fprintf(stdout, COLOR_GREEN "[INFO] " COLOR_RESET fmt "", ##__VA_ARGS__); \
} while (0)

#define DEBUG_FUNC(fmt, ...) do { \
    fprintf(stdout, COLOR_CYAN "[FUNC] %s: " COLOR_RESET fmt "", __func__, ##__VA_ARGS__); \
} while (0)

#define DEBUG_ERR(fmt, ...) do { \
    fprintf(stderr, COLOR_RED "[ERROR] %s: (%s) " COLOR_RESET fmt "", __func__, strerror(errno), ##__VA_ARGS__); \
} while (0)

/**
 * @brief Macro to check if structure is instanced.
 */
#define UIDQ_STRUCT_CHECK_INSTANCE(ptr, type) \
   do { \
      if (!(ptr) || !(ptr)->instance) { \
         DEBUG_ERR("Bitmask is not initialized\n"); \
         type; \
      } \
   } while (0)

/**
 * @brief Macro to check if structure is initialized.
 */
#define UIDQ_STRUCT_CHECK_INIT(ptr, type) \
   do { \
      if (!(ptr) || !(ptr)->initialized) { \
         DEBUG_ERR("Bitmask is not initialized\n"); \
         type; \
      } \
   } while (0)

void uidq_free_pointer(void **ptr);

int uidq_round_pow(int number, int exp);

#endif
