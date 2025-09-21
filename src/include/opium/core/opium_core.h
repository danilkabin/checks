#ifndef OPIUM_CORE_INCLUDE_H
#define OPIUM_CORE_INCLUDE_H

#include <limits.h>
#include <stdint.h>

/* Detect pointer size: 32-bit vs 64-bit */
#if defined(__x86_64__) || defined(__ppc64__)
#define OPIUM_PTR_SIZE 8
#else
#define OPIUM_PTR_SIZE 4
#endif

/* Return codes for functions */
#define OPIUM_RET_OK    0  /* Success */
#define OPIUM_RET_ERR  -1  /* Generic error */
#define OPIUM_RET_FULL -2  /* Resource full / cannot allocate */

/* Limits for optimized memcpy / memset routines */
#define OPIUM_MEMCPY_LIMIT 2048
#define OPIUM_MEMSET_LIMIT 2048

/* Utility macros */
#define opium_min(a,b) ((a) < (b) ? (a) : (b))
#define opium_max(a,b) ((a) > (b) ? (a) : (b))

/* Branch prediction hints */
#define opium_likely(exp)   __builtin_expect(!!(exp), 1)
#define opium_unlikely(exp) __builtin_expect(!!(exp), 0)

/* Fixed-width integer types with short aliases */
typedef int8_t   opium_byte_t;
typedef int16_t  opium_short_t;  
typedef int32_t  opium_int_t;
typedef int64_t  opium_long_t;

typedef uint8_t  opium_ubyte_t;
typedef uint16_t opium_ushort_t;
typedef uint32_t opium_uint_t;
typedef uint64_t opium_ulong_t;

/* Key type (1 byte) */
typedef opium_byte_t opium_key_t;

/* Boolean flag type */
typedef opium_byte_t opium_flag_t;

/* Boolean flag utilities */
#define opium_flag_true(flag)    ((flag) = 1)
#define opium_flag_false(flag)   ((flag) = 0)
#define opium_flag_istrue(flag)  ((flag) == 1)
#define opium_flag_isfalse(flag) ((flag) == 0)

/* Standard chunk size */
#define OPIUM_CHUNK_SIZE sizeof(opium_uint_t)

/* Utility to print CPU info */
void opium_cpuinfo(void);

#endif /* OPIUM_CORE_INCLUDE_H */
