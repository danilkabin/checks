#ifndef UIDQ_MASTER_H
#define UIDQ_MASTER_H

#include <stdint.h>
#include <sys/types.h>

#include "core/uidq_core.h"
#include "core/uidq_socket.h"
#include "core/uidq_slab.h"
#include "master/uidq_master_proc.h"

typedef struct {
   int unix_fd;
   char *addr;
   uint16_t port;

   size_t max_peers;
   size_t max_queue;

   char *worker_bin_path;
   char **worker_bin_argv;

   uidq_process_ctl_t *proc_ctl;
   uidq_sock_ctl_t *sock_ctl;

} uidq_master_t;

uidq_master_t *uidq_master_init();
void uidq_master_exit(uidq_master_t *master);

#endif
