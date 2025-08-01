#ifndef UIDQ_MASTER_H
#define UIDQ_MASTER_H

#include <stdint.h>
#include <sys/types.h>

#include "uidq_master_process.h"
#include "uidq_master_config.h"

#define UIDQ_PROCESS_WORKER_PATH "build/bin/process_worker"
#define UIDQ_PROCESS_WORKER_NAME "process_worker"

typedef struct {
   uidq_proc_ctl_t *process_handler;
   uidq_master_conf_t *conf;
} uidq_master_t;

int uidq_master_init();
void uidq_master_exit();

#endif
