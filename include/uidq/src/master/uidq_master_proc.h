#ifndef UIDQ_MASTER_PROC_H
#define UIDQ_MASTER_PROC_H

#include "core/uidq_core.h"
#include "core/uidq_slab.h"

#include <sys/types.h>
#include <time.h>

typedef struct {
   char *path;
   char *name;
   char *const *argv;
   char *const *envp;
} uidq_exec_var_t;

typedef struct {
    pid_t pid;
    int index;
    int status;
    int running;
    int respawn_count;

    time_t start_time;
    time_t heartbeat;

    char name[32];
} uidq_process_t;

typedef struct {
   uidq_slab_t *processes;
   int count;
   int max_count;
} uidq_process_ctl_t;

int uidq_affinity_count(void);
int uidq_affinity_get(void);
int uidq_affinity_set(int core);

uidq_process_t *uidq_process_init(uidq_process_ctl_t *ctl);
void uidq_process_exit(uidq_process_ctl_t *ctl, uidq_process_t *process);

uidq_process_ctl_t *uidq_process_ctl_init(uidq_log_t *log, int max_count);
void uidq_process_ctl_exit(uidq_process_ctl_t *ctl);

#endif
