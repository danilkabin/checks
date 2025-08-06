#ifndef UIDQ_H
#define UIDQ_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "uidq_version.h"
#include "uidq_paths.h"
#include "uidq_types.h"
#include "uidq_log.h"
#include "uidq_master.h"
#include "uidq_utils.h"

enum {
   UIDQ_FLAG_DAEMON = (1 << 0),
   UIDQ_FLAG_ONION, 
   UIDQ_FLAG_IPV6
} uidq_flags_t;

typedef enum {
    UIDQ_LOG_NONE    = 0,
    UIDQ_LOG_ERROR   = 1 << 0,
    UIDQ_LOG_WARNING = 1 << 1,
    UIDQ_LOG_INFO    = 1 << 2,
} uidq_flags_log_t;

typedef struct {
   int flags;
   int daemon;
   int onion;

   int timeout;
   int read_timeout;
   int write_timeout;

   size_t read_buff_size;
   size_t write_buff_size;

   char *username;
   char *hostname;
   
   uidq_log_t *log_ctl;

   uidq_master_t *master;
} uidq_t;

int uidq_get_log_level(uidq_t *uidq);

#endif

