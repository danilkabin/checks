#ifndef OPIUM_CORE_INCLUDE_H
#define OPIUM_CORE_INCLUDE_H

#include "opium.h"

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

/* Fixed-width integer types with short aliases */
typedef int8_t   opium_s8_t;
typedef int16_t  opium_s16_t;  
typedef int32_t  opium_s32_t;
typedef int64_t  opium_s64_t;

typedef uint8_t  opium_u8_t;
typedef uint16_t opium_u16_t;
typedef uint32_t opium_u32_t;
typedef uint64_t opium_u64_t;

typedef opium_s8_t opium_key_t;
typedef opium_s8_t opium_flag_t;
typedef opium_s32_t  opium_fd_t;
typedef opium_s32_t  opium_socket_fd_t;
typedef cpu_set_t    opium_cpuset_t;

typedef uid_t        opium_uid_t;
typedef gid_t        opium_gid_t;
typedef off_t        opium_off_t;
typedef mode_t       opium_mode_t;
typedef time_t       opium_time_t;

typedef FILE         opium_ofile_t;

typedef int          opium_err_t;

/* Boolean flag utilities */
#define opium_flag_true(flag)    ((flag) = 1)
#define opium_flag_false(flag)   ((flag) = 0)
#define opium_flag_istrue(flag)  ((flag) == 1)
#define opium_flag_isfalse(flag) ((flag) == 0)

/* Standard chunk size */
#define OPIUM_CHUNK_SIZE sizeof(opium_uint_t)

/* Definitions */

typedef struct opium_log_s         opium_log_t;
typedef struct opium_list_head_s   opium_list_head_t;
typedef struct opium_slab_s        opium_slab_t;
typedef struct opium_arena_s       opium_arena_t;
//typedef struct opium_hash_s        opium_hash_t;
typedef struct opium_rbt_s         opium_rbt_t;
typedef struct opium_thread_s      opium_thread_t;
typedef struct opium_event_s       opium_event_t;

/* Includes */
#include "opium_log.h"
#include "opium_list.h"
#include "opium_hashfuncs.h"
#include "opium_alloc.h"

#include "opium_slab.h"
#include "opium_arena.h"

#include "opium_rbt.h"
#include "opium_thread.h"
#include "opium_event.h"

#include "opium_network.h"
#include "opium_server.h"
#include "opium_connection.h"

/* Utility macros */
#define opium_min(a,b) ((a) < (b) ? (a) : (b))
#define opium_max(a,b) ((a) > (b) ? (a) : (b))

/* Branch prediction hints */
#define opium_likely(exp)   __builtin_expect(!!(exp), 1)
#define opium_unlikely(exp) __builtin_expect(!!(exp), 0)

/* Utility to print CPU info */
void opium_cpuinfo(void);

#endif /* OPIUM_CORE_INCLUDE_H */
