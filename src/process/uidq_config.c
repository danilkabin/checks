#include "process/uidq_config.h"
#include "core/uidq_alloc.h"
#include "core/uidq_log.h"
#include <stdlib.h>

uidq_proc_conf_t uidq_proc_conf;

int uidq_proc_conf_init() {
   uidq_proc_conf.max_peers = UIDQ_CONF_MAX_PEERS;
   uidq_proc_conf.max_queue = UIDQ_CONF_MAX_QUEUE; 
   return 1;
}

void uidq_proc_conf_clean() {

}
