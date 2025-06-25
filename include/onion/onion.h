#ifndef ONION_H
#define ONION_H

#include <stddef.h>

#define ONION_HTTP_LINE_METHOD_MAX_SIZE     32
#define ONION_HTTP_LINE_URL_MAX_SIZE       464
#define ONION_HTTP_LINE_VERSION_MAX_SIZE    16

#define ONION_HTTP_LINE_MAX_SIZE          (ONION_HTTP_LINE_METHOD_MAX_SIZE + ONION_HTTP_LINE_URL_MAX_SIZE + ONION_HTTP_LINE_VERSION_MAX_SIZE)

#define ONION_HTTP_HEADER_NAME_SIZE        64
#define ONION_HTTP_HEADER_VALUE_SIZE       128
#define ONION_HTTP_MAX_HEADERS             24

#define ONION_HTTP_MAX_HEADER_SIZE        (ONION_HTTP_MAX_HEADERS * (ONION_HTTP_HEADER_NAME_SIZE + ONION_HTTP_HEADER_VALUE_SIZE))
#define ONION_HTTP_INITIAL_HEADER_SIZE    (ONION_HTTP_MAX_HEADER_SIZE / 8)

#define ONION_HTTP_INIT_BODY_SIZE      256
#define ONION_HTTP_MAX_BODY_SIZE          8192

#define ONION_HTTP_INIT_MESSAGE_SIZE  (ONION_HTTP_LINE_MAX_SIZE + ONION_HTTP_INITIAL_HEADER_SIZE + ONION_HTTP_INITIAL_BODY_SIZE)
#define ONION_HTTP_MAX_MESSAGE_SIZE      (ONION_HTTP_LINE_MAX_SIZE + ONION_HTTP_MAX_HEADER_SIZE + ONION_HTTP_MAX_BODY_SIZE)

#define ONION_HTTP_MAX_REQUESTS             8
#define ONION_HTTP_MAX_REQUESTS_SIZE      (ONION_HTTP_MAX_REQUESTS * ONION_HTTP_MAX_MESSAGE_SIZE)

#define ONION_MAX_PEER_PER_COUNT 16
#define ONION_MAX_PEER_QUEUE_CAPABLE 16

typedef struct {
   int core_count;

   int max_peer_per_core;
   int max_peer_queue_capable;
   
   int http_max_requests;

   size_t http_line_method_max_size;
   size_t http_line_url_max_size;
   size_t http_line_version_max_size;

   size_t http_header_name_size;
   size_t http_header_value_size;
   size_t http_header_max_headers;

   size_t http_init_body_size;
   size_t http_max_body_size;
} onion_config_t;

//extern onion_config_t onion_config;

int onion_config_init(onion_config_t *onion_config);
void onion_config_exit();

#endif
