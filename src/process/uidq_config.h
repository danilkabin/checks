#ifndef UIDQ_CONFIG_H
#define UIDQ_CONFIG_H

#include <stddef.h>

#define UIDQ_CONF_MAX_PEERS 1024 
#define UIDQ_CONF_MAX_QUEUE 32

typedef struct {
   size_t max_peers;
   size_t max_queue;
} uidq_proc_conf_t;

extern uidq_proc_conf_t uidq_proc_conf;

int uidq_proc_conf_init();
void uidq_proc_conf_clean();

#endif
