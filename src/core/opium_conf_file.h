#ifndef OPIUM_CONF_FILE_INCLUDE_H
#define OPIUM_CONF_FILE_INCLUDE_H

#include "core/opium_core.h"

/* Server configuration structure */
typedef struct opium_conf_server_s {
    opium_string_t server_name;
    opium_string_t ip;
    opium_uint_t   port;
} opium_conf_server_t;

/* Worker configuration structure */
typedef struct opium_conf_worker_s {
    opium_string_t  name;
    opium_flag_t    daemon;
    opium_uint_t    worker_processes;
    opium_flag_t    cpu_affinity_auto;
    opium_uint_t    cpu_affinity_n;
} opium_conf_worker_t;

/* Main configuration structure */
typedef struct opium_conf_s {
    opium_conf_server_t server_conf;
    opium_array_t       workers;
    opium_arena_t       arena;
} opium_conf_t;

/* File handling structure */
typedef struct opium_open_file_s {
    opium_ofile_t    *object;
    u_char           *path;
    void             *data;
    size_t            len;
    opium_conf_t      conf;
    opium_pool_t     *pool;
    opium_array_t     array;
    opium_log_t      *log;
} opium_open_file_t;

#endif /* OPIUM_CONF_FILE_INCLUDE_H */
