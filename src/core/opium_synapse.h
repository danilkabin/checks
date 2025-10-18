#ifndef OPIUM_CYCLE_INCLUDE_H
#define OPIUM_CYCLE_INCLUDE_H 

#include "core/opium_core.h"

typedef struct {
   opium_flag_t    daemon;
   opium_flag_t    master;

   opium_uint_t    worker_processes;

   char           *name;

   opium_flag_t    cpu_affinity_auto;
   opium_uint_t    cpu_affinity_n;
   opium_cpuset_t *cpu_affinity;
} opium_synapse_stats_t;

struct opium_synapse_s {
   opium_synapse_stats_t stats;

   opium_uint_t          connection_n;
   opium_uint_t          files_n;

   opium_log_t          *log;
};

#endif /* OPIUM_CYCLE_INCLUDE_H  */
