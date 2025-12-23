#ifndef OPIUM_INCLUDE_H
#define OPIUM_INCLUDE_H

#if defined (__linux)
#include "os/unix/opium_lib.h"
#endif

int opium_is_little_endian(void);
size_t opium_round_of_two(size_t x);
size_t opium_log2(size_t x);

void opium_debug_point(void);

#endif /* OPIUM_INCLUDE_H */
