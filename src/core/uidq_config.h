#ifndef UIDQ_CONFIG_H
#define UIDQ_CONFIG_H

#include <stdint.h>

#define UIDQ_DEFAULT_ADDR "127.0.0.1"
#define UIDQ_DEFAULT_PORT 8080
#define UIDQ_DEFAULT_MAX_PEERS 160
#define UIDQ_DEFAULT_MAX_QUEUE 16

#define UIDQ_DEFAULT_LOG_FILE ""

#define UIDQ_ADDR_SIZE 64

typedef struct {
   char *name;

} uidq_command_t;

typedef struct {
   char addr[UIDQ_ADDR_SIZE];
   uint32_t port;
   uint32_t max_peers;
   uint32_t max_queue;
   const char *log_file;
} uidq_server_config_t;

#endif
