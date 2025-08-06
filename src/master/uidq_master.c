#include <sys/socket.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "master/uidq_master.h"
#include "master/uidq_master_proc.h"
#include "master/uidq_master_ipc.h"
#include "core/uidq_alloc.h"
#include "core/uidq_slab.h"
#include "core/uidq_utils.h"
#include "core/uidq_log.h"

static int uidq_master_unix_fd_init(int core_count) {
   uidq_sock_conf_t conf;
   memset(&conf, 0, sizeof(conf));

   conf.domain = AF_UNIX;
   conf.type = SOCK_STREAM;
   conf.protocol = 0;
   conf.queue_capable = core_count;

   memcpy(conf.sock_un.sun_path, "/tmp/uidqsock.sock", sizeof(conf.sock_un.sun_path) + 1);

   return uidq_sock_init(&conf);
}

uidq_master_t *uidq_master_init() {
   size_t core_count = uidq_affinity_count();
   uidq_master_t *master = uidq_calloc(sizeof(uidq_master_t));
   if (!master) {
      UIDQ_ERR("Failed to allocate master controller.\n");
      return NULL;
   }

   master->addr = strdup("127.0.0.1");
   master->port = 8080;

   master->unix_fd = uidq_master_unix_fd_init(core_count);
   if (master->unix_fd < 0) {
      UIDQ_ERR("Failed to allocate unix fd.\n");
      goto free_master;
   }

   master->proc_ctl = uidq_process_ctl_init(core_count); 
   if (!master->proc_ctl) {
      UIDQ_ERR("Failed to allocate process controller.\n");
      goto free_master;
   }

   uidq_process_init(master->proc_ctl);

   return master;

free_master:
   free(master);
   return NULL;
}

void uidq_master_exit(uidq_master_t *master) {
   if (!master) return;
   //free(master->worker_pids);

   if (master->addr) {
      free(master->addr);
      master->addr = NULL;
   }

   if (master->unix_fd) {
      close(master->unix_fd);
      master->unix_fd = -1;
   }

   free(master);
}
