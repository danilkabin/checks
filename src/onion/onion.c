#include "onion.h"
#include "sup.h"
#include "utils.h"
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

onion_config_t onion_config = {0};

int onion_config_init(onion_config_t *user_cfg) {
   long core_count = sysconf(_SC_NPROCESSORS_CONF);
   if (core_count < 1) {
      DEBUG_FUNC("onion_config_init: no CPU cores found (core_count = %ld)\n", core_count);
      return -1;
   }
   onion_config.core_count = core_count;

   long sched_core = 0;
   if (onion_cpu_set_core(pthread_self(), sched_core) == -1) {
      DEBUG_FUNC("failed to set CPU affinity.\n");
      return -1;
   }
   onion_config.sched_core = sched_core;

   if (sched_core >= core_count) {
      DEBUG_FUNC("sched_core (%ld) >= core_count (%ld)\n", sched_core, core_count);
      return -1;
   }

   onion_config.max_peer_per_core = (user_cfg && user_cfg->max_peer_per_core >= 1)
      ? user_cfg->max_peer_per_core
      : ONION_MAX_PEER_PER_COUNT;

   onion_config.max_peer_queue_capable = (user_cfg && user_cfg->max_peer_queue_capable > 0)
      ? user_cfg->max_peer_queue_capable
      : ONION_MAX_PEER_QUEUE_CAPABLE;

   onion_config.http_max_requests = (user_cfg && user_cfg->http_max_requests > 0)
      ? user_cfg->http_max_requests
      : ONION_HTTP_MAX_REQUESTS;

   onion_config.http_line_method_max_size = (user_cfg && user_cfg->http_line_method_max_size > 0)
      ? user_cfg->http_line_method_max_size
      : ONION_HTTP_LINE_METHOD_MAX_SIZE;

   onion_config.http_line_url_max_size = (user_cfg && user_cfg->http_line_url_max_size > 0)
      ? user_cfg->http_line_url_max_size
      : ONION_HTTP_LINE_URL_MAX_SIZE;

   onion_config.http_line_version_max_size = (user_cfg && user_cfg->http_line_version_max_size > 0)
      ? user_cfg->http_line_version_max_size
      : ONION_HTTP_LINE_VERSION_MAX_SIZE;

   onion_config.http_header_name_size = (user_cfg && user_cfg->http_header_name_size > 0)
      ? user_cfg->http_header_name_size
      : ONION_HTTP_HEADER_NAME_SIZE;

   onion_config.http_header_value_size = (user_cfg && user_cfg->http_header_value_size > 0)
      ? user_cfg->http_header_value_size
      : ONION_HTTP_HEADER_VALUE_SIZE;

   onion_config.http_header_max_headers = (user_cfg && user_cfg->http_header_max_headers > 0)
      ? user_cfg->http_header_max_headers
      : ONION_HTTP_MAX_HEADERS;

   onion_config.http_init_body_size = (user_cfg && user_cfg->http_init_body_size > 0)
      ? user_cfg->http_init_body_size
      : ONION_HTTP_INIT_BODY_SIZE;

   onion_config.http_max_body_size = (user_cfg && user_cfg->http_max_body_size > 0)
      ? user_cfg->http_max_body_size
      : ONION_HTTP_MAX_BODY_SIZE;

   return 0;
}
