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
typedef int8_t   opium_byte_t;
typedef int16_t  opium_short_t;  
typedef int32_t  opium_int_t;
typedef int64_t  opium_long_t;

typedef uint8_t  opium_ubyte_t;
typedef uint16_t opium_ushort_t;
typedef uint32_t opium_uint_t;
typedef uint64_t opium_ulong_t;

typedef opium_byte_t opium_key_t;
typedef opium_byte_t opium_flag_t;
typedef opium_int_t  opium_fd_t;
typedef opium_int_t  opium_socket_fd_t;
typedef cpu_set_t    opium_cpuset_t;

typedef uid_t        opium_uid_t;
typedef gid_t        opium_gid_t;
typedef off_t        opium_off_t;
typedef mode_t       opium_mode_t;
typedef time_t       opium_time_t;

typedef FILE         opium_ofile_t;

/* Boolean flag utilities */
#define opium_flag_true(flag)    ((flag) = 1)
#define opium_flag_false(flag)   ((flag) = 0)
#define opium_flag_istrue(flag)  ((flag) == 1)
#define opium_flag_isfalse(flag) ((flag) == 0)

/* Standard chunk size */
#define OPIUM_CHUNK_SIZE sizeof(opium_uint_t)

/* Definitions */

typedef struct opium_log_s         opium_log_t;
typedef struct opium_string_s      opium_string_t;
typedef struct opium_list_head_s   opium_list_head_t;
typedef struct opium_slab_s        opium_slab_t;
typedef struct opium_arena_s       opium_arena_t;
typedef struct opium_bitmask_s     opium_bitmask_t;
typedef struct opium_pool_s        opium_pool_t;
typedef struct opium_array_s       opium_array_t;
typedef struct opium_hash_s        opium_hash_t;
typedef struct opium_rbt_s         opium_rbt_t;

typedef struct opium_conf_file_s   opium_conf_file_t ;
typedef struct opium_command_s     opium_command_t;
typedef struct opium_conf_server_s opium_conf_server_t;
typedef struct opium_conf_worker_s opium_conf_worker_t;
typedef struct opium_conf_s        opium_conf_t;

typedef struct opium_synapse_s     opium_synapse_t;
typedef struct opium_open_file_s   opium_open_file_t;

typedef struct opium_file_s        opium_file_t;
typedef struct opium_path_s        opium_path_t;

typedef struct opium_listening_s   opium_listening_t;
typedef struct opium_connection_s  opium_connection_t;

/* Includes */
#include "opium_log.h"
#include "opium_list.h"
#include "opium_string.h"
#include "opium_hashfuncs.h"
#include "opium_alloc.h"

#include "opium_slab.h"
#include "opium_arena.h"
#include "opium_bitmask.h"
#include "opium_pool.h"
#include "opium_array.h"

#include "opium_rbt.h"
#include "opium_hash.h"

#include "opium_conf_file_parser.h"
#include "opium_conf_file.h"

#include "opium_synapse.h"
#include "opium_file.h"
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
