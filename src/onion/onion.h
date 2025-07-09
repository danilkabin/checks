#ifndef ONION_H
#define ONION_H

#include <stddef.h>

#include "device.h"
#include "epoll.h"
#include "net.h"
#include "http.h"

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

typedef struct {
   onion_server_conf_triad_t server_triad; 
   onion_http_conf_t http;
} onion_config_t;


onion_config_t *onion_config_init();
void onion_config_exit();

#endif
