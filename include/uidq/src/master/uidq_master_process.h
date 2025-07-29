#ifndef UIDQ_MASTER_PROCESS_H
#define UIDQ_MASTER_PROCESS_H

#include "uidq_slab.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
   char *path;
   char *name;
   char *const *argv;
   char *const *envp;
} uidq_exec_var_t;

typedef struct {
   uint32_t pos;
   pid_t pid;
} uidq_proc_t;

typedef struct {
   uint32_t count;
   uint32_t max_count;
   uidq_slab_t *processes;

   bool initialized;
} uidq_proc_ctl_t;

pid_t uidq_process_create(void *data);
void uidq_process_remove(pid_t fork_pid);

uidq_proc_t *uidq_proc_ctl_add(uidq_proc_ctl_t *ctl, void *data);
void uidq_proc_ctl_del(uidq_proc_ctl_t *ctl, uidq_proc_t *proc);

uidq_proc_ctl_t *uidq_proc_ctl_init(uint32_t count, uint32_t max_count);
void uidq_proc_ctl_exit(uidq_proc_ctl_t *ctl);

#endif
