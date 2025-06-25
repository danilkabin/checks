#include "onion.h"
#include "utils.h"

#include <unistd.h>
#include <stdio.h>

int onion_config_init(onion_config_t *onion_config) {
   int core_count = sysconf(_SC_NPROCESSORS_CONF);
   if (core_count < 1) {
      DEBUG_FUNC("No cores found!\n");   
      return -1;
   }

   onion_config->core_count = core_count;

   onion_config->max_peer_per_core = ONION_MAX_PEER_PER_COUNT;
   onion_config->max_peer_queue_capable = ONION_MAX_PEER_QUEUE_CAPABLE; 

   onion_config->http_max_requests = ONION_HTTP_MAX_REQUESTS;

   onion_config->http_line_method_max_size = ONION_HTTP_LINE_METHOD_MAX_SIZE;
   onion_config->http_line_url_max_size = ONION_HTTP_LINE_URL_MAX_SIZE;
   onion_config->http_line_version_max_size = ONION_HTTP_LINE_VERSION_MAX_SIZE;

   onion_config->http_header_name_size = ONION_HTTP_HEADER_NAME_SIZE;
   onion_config->http_header_value_size = ONION_HTTP_HEADER_VALUE_SIZE;
   onion_config->http_header_max_headers = ONION_HTTP_MAX_HEADERS;

   onion_config->http_init_body_size = ONION_HTTP_INIT_BODY_SIZE;
   onion_config->http_max_body_size = ONION_HTTP_MAX_BODY_SIZE;
   return 0; 
}
