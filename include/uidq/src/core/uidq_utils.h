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

void uidq_free_pointer(void **ptr);

int uidq_round_pow(int number, int exp);

#endif
