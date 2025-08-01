#ifndef UIDQ_MASTER_CONF_H
#define UIDQ_MASTER_CONF_H

#define UIDQ_DEFAULT_IP_ADDRESS "127.0.0.1"
#define UIDQ_IP_ADDRESS_SIZE 64

#define UIDQ_DEFAULT_PORT 8080
#define UIDQ_DEFAULT_MAX_PEERS 160
#define UIDQ_DEFAULT_MAX_QUEUE 16

typedef struct {
   int workers_max;
   int sched_core;

    char ip_address[UIDQ_IP_ADDRESS_SIZE];
    int port;
    int max_queue;
} uidq_master_conf_arg_t;

typedef struct {
   uidq_master_conf_arg_t *args;
   int workers;
   int current_peers;
} uidq_master_conf_t;

#endif
