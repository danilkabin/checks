#ifndef ONION_H
#define ONION_H

#include <stddef.h>

#define ONION_HTTP_LINE_MAX_SIZE          (ONION_HTTP_LINE_METHOD_MAX_SIZE + ONION_HTTP_LINE_URL_MAX_SIZE + ONION_HTTP_LINE_VERSION_MAX_SIZE)

#define ONION_HTTP_MAX_HEADER_SIZE        (ONION_HTTP_MAX_HEADERS * (ONION_HTTP_HEADER_NAME_SIZE + ONION_HTTP_HEADER_VALUE_SIZE))
#define ONION_HTTP_INITIAL_HEADER_SIZE    (ONION_HTTP_MAX_HEADER_SIZE / 8)

#define ONION_HTTP_INIT_MESSAGE_SIZE  (ONION_HTTP_LINE_MAX_SIZE + ONION_HTTP_INITIAL_HEADER_SIZE + ONION_HTTP_INITIAL_BODY_SIZE)
#define ONION_HTTP_MAX_MESSAGE_SIZE      (ONION_HTTP_LINE_MAX_SIZE + ONION_HTTP_MAX_HEADER_SIZE + ONION_HTTP_MAX_BODY_SIZE)

#define ONION_HTTP_MAX_REQUESTS_SIZE      (ONION_HTTP_MAX_REQUESTS * ONION_HTTP_MAX_MESSAGE_SIZE)

typedef int (*qsort_compare)(const void *a, const void *b);

typedef enum {
   ONION_INT_TYPE,
   ONION_STRING_TYPE
} onion_conf_val_type;

#define ONION_ENTRY_NAME_SIZE 64
#define ONION_ENTRY_VALUE_SIZE 64
#define ONION_ENTRY_LINE_SIZE ONION_ENTRY_NAME_SIZE + ONION_ENTRY_VALUE_SIZE

typedef struct {
   char name[ONION_ENTRY_NAME_SIZE];
   char value[ONION_ENTRY_VALUE_SIZE];
} onion_conf_pair_t;

typedef struct {
   char key[ONION_ENTRY_LINE_SIZE];
   onion_conf_val_type type;
   void *ptr;
} onion_conf_entry_t;

#define ONION_IP_ADDRESS_SIZE 64
#define ONION_DEFAULT_IP_ADDRESS "127.0.0.1"
#define ONION_DEFAULT_PORT 8080
#define ONION_DEFAULT_MAX_PEERS 16
#define ONION_DEFAULT_MAX_QUEUE 16
#define ONION_DEFAULT_TIMEOUT 1200

typedef struct {
    char ip_address[ONION_IP_ADDRESS_SIZE];
    int port;
    int max_peers;
    int max_queue;
    int timeout;
} onion_net_conf_t;

#define ONION_DEFAULT_CORE_SCHED 0

typedef struct {
    int count;
    int sched;
    int worker_count;
} onion_core_conf_t;

#define ONION_DEFAULT_METHOD_SIZE 32
#define ONION_DEFAULT_URL_SIZE 464
#define ONION_DEFAULT_VERSION_SIZE 16
#define ONION_DEFAULT_HEADER_NAME_SIZE 64
#define ONION_DEFAULT_HEADER_VALUE_SIZE 128
#define ONION_DEFAULT_MAX_HEADERS 24
#define ONION_DEFAULT_INIT_BODY_SIZE 256
#define ONION_DEFAULT_MAX_BODY_SIZE 8192
#define ONION_DEFAULT_MAX_REQUESTS 8

typedef struct {
    size_t method_size;
    size_t url_size;
    size_t version_size;

    size_t header_name_size;
    size_t header_value_size;
    size_t max_headers;

    size_t init_body_size;
    size_t max_body_size;

    int max_requests;
} onion_http_conf_t;

typedef struct {
    onion_core_conf_t core;
    onion_net_conf_t net;
    onion_http_conf_t http;
} onion_config_t;


onion_config_t *onion_config_init();
void onion_config_exit();

#endif
