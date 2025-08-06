#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "core/uidq_alloc.h"
#include "core/uidq_log.h"
#include "core/uidq_utils.h"
#include "master/uidq.h"
#include "master/uidq_master.h"

int uidq_flags(uidq_t *uidq) {
   return uidq->flags;
}

uidq_log_t *uidq_log_ctl(uidq_t *uidq) {
   return uidq->log_ctl;
}

void uidq_set_flags(uidq_t *uidq) {
   int flags = uidq_flags(uidq);
   uidq->daemon = flags & UIDQ_FLAG_DAEMON ? 1 : 0; 
   uidq->onion = flags & UIDQ_FLAG_ONION ? 1 : 0; 
}

void uidq_set_name(uidq_t *uidq, char *prefix) {
   if (uidq->username) {
      free(uidq->username);
   } 
   char buff[128];
   snprintf(buff, sizeof(buff), "%s-%ld-%d", prefix, time(NULL), getpid());
   uidq->username = strdup(buff); 
}

void uidq_set_host_name(uidq_t *uidq, char *hostname) {
   if (uidq->hostname) {
      free(uidq->hostname);
   }
   if (!hostname) {
      char name[256];
      if (gethostname(name, sizeof(name)) == 0) {
         uidq->hostname = strdup(name);
      } else {
         uidq->hostname = strdup("unknown");
      }
   } else {
      uidq->hostname = strdup(hostname);
   }
}

uidq_t *uidq_init(int flags) {
   uidq_t *uidq = uidq_calloc(sizeof(uidq_t));
   if (!uidq) {
      return NULL;
   }

   uidq->log_ctl = uidq_log_init(NULL, NULL, NULL);
   if (!uidq->log_ctl) {
      goto free_uidq;
   }

   uidq_set_flags(uidq);

   uidq->timeout = 5000;
   uidq->read_timeout = 60 * 100000;
   uidq->write_timeout = 30 * 1000;

   uidq->read_buff_size = 8192;
   uidq->write_buff_size = 8192;

   uidq_set_name(uidq, "uidq");
   uidq_set_host_name(uidq, NULL);

   if (uidq->daemon) {

   }

   if (uidq->onion) {

   }

   uidq->master = uidq_master_init();
   if (!uidq->master) {
      UIDQ_ERR("Failed to allocate uidq master controller.\n");
      goto free_uidq;
   }

   UIDQ_DEBUG("UIDQ initialized:\n" "  Version   : %s\n" "  Username  : %s\n" "  Hostname  : %s\n",
         UIDQ_VER, uidq->username ? uidq->username : "(null)", uidq->hostname ? uidq->hostname : "(null)");

   return uidq;
free_uidq:
   //   uidq_exit(uidq);
   return NULL;
}

void uidq_exit(uidq_t *uidq) {
   if (!uidq) return;

   if (uidq->master) {
      uidq_master_exit(uidq->master);
      uidq->master = NULL;
   }

   if (uidq->log_ctl) {
      uidq_log_exit(uidq->log_ctl);
      uidq->log_ctl = NULL;
   }

   free(uidq);
}

int main() {
   DEBUG_FUNC("heellooo!\n");
   uidq_log_t *log = uidq_log_init(NULL, NULL, NULL);

   uidq_t *uidq = uidq_init(-1);
   return 0;
}
