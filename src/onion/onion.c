#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "device.h"
#include "http.h"
#include "onion.h"
#include "sup.h"
#include "utils.h"

const char *config_path = "build/config.ini";
const char *default_phrase = "default";

int onion_config_cmp(const void *a, const void *b) {
   onion_conf_entry_t *sigma_a = (onion_conf_entry_t*)a;
   onion_conf_entry_t *sigma_b = (onion_conf_entry_t*)b;
   return strcmp(sigma_a->key, sigma_b->key);
}

void onion_conf_entry_set(onion_conf_entry_t *entry, void *data) {
   if (entry->type == ONION_INT_TYPE) {
      int number = atoi(data);
      if (strncasecmp(data, default_phrase, strlen(default_phrase)) == 0) {
         number = -1;
      }   
      *(int*)entry->ptr = number;
   } else if (entry->type == ONION_STRING_TYPE) {
      strncpy((char*)entry->ptr, data, ONION_ENTRY_VALUE_SIZE);
   }
}

void onion_conf_pair_init(onion_conf_pair_t *pair, const char *name, size_t name_size, const char *value, size_t value_size) {
   if (name && name_size > 0) {
      strncpy(pair->name, name, name_size - 1);
      pair->name[name_size - 1] = '\0';
   } else {
      pair->name[0] = '\0';
   }

   if (value && value_size > 0) {
      strncpy(pair->value, value, value_size - 1);
      pair->value[value_size - 1] = '\0';
   } else {
      pair->value[0] = '\0';
   }
}

int onion_buff_trim_sf(char *data, size_t len, char *out, size_t out_size) {
   while (len > 0 && isspace((unsigned char)*data)) {
      data = data + 1;
      len = len - 1;
   }

   while (len > 0 && isspace((unsigned char)data[len - 1])) {
      len = len - 1;
   }

   if (len >= out_size) {
      len = out_size - 1;
   }

   memcpy(out, data, len);
   out[len] = '\0';
   return 0;
}

int onion_buff_trim(char *data, size_t data_size, onion_conf_pair_t *pair) {
   char *pos = data;

   char *equal = onion_chacha(data, data_size, '=');
   char *line_end = onion_chacha(data, data_size, '\n');
   if (!equal || !line_end) {
      return -1;
   }

   char *name_start = pos;
   size_t name_len = equal - pos;

   char *value_start = equal + 1;
   size_t value_len = line_end - value_start;

   if (name_len >= sizeof(pair->name) || value_len >= sizeof(pair->value)) { 
      return -1;
   }

   onion_buff_trim_sf(name_start, name_len, pair->name, sizeof(pair->name));
   onion_buff_trim_sf(value_start, value_len, pair->value, sizeof(pair->value));
   return 0;
}

int onion_read_ini(const char *file_name, onion_conf_entry_t *entry, size_t capable, size_t size) {
   FILE *file = fopen(file_name, "r");
   if (!file) {
      return -1;
   }

   size_t line_size = ONION_ENTRY_LINE_SIZE; 
   char line[line_size];

   while (fgets(line, sizeof(line), file)) {
      if (line[0] == '#' || line[0] == '\n') {
         continue;
      }
      onion_conf_pair_t pair;
      onion_conf_pair_init(&pair, NULL, ONION_ENTRY_NAME_SIZE, NULL, ONION_ENTRY_VALUE_SIZE); 

      onion_buff_trim(line, line_size, &pair);

      onion_conf_entry_t key = {0};
      snprintf(key.key, sizeof(key.key), "%s", pair.name); 
      onion_conf_entry_t *f_entry = bsearch(&key, entry, capable, size, onion_config_cmp);
      if (!f_entry) {
         continue;
      }

      onion_conf_entry_set(f_entry, &pair.value);
   }
   return 0;
}

onion_config_t *onion_config_init() {
   int ret;
   onion_config_t *config = malloc(sizeof(*config));
   if (!config) {
      DEBUG_FUNC("Config initialization failed.\n");
      goto unsucccessfull;
   }
   if (onion_conf_triad_init(&config->server_triad) == -1) {
      DEBUG_FUNC("Triad initialization failed.\n");
      goto unsucccessfull;
   }
   ONION_UNPACK_TRIAD(&config->server_triad);

   onion_conf_entry_t entry[] = {
      // CORE
      {.key = "core_count",   .type = ONION_INT_TYPE, .ptr = &core_conf->count},
      {.key = "core_sched",   .type = ONION_INT_TYPE, .ptr = &core_conf->sched},
      {.key = "worker_count", .type = ONION_INT_TYPE, .ptr = &core_conf->worker_count},

      // NET
      {.key = "ip_address", .type = ONION_STRING_TYPE, .ptr = &net_conf->ip_address},
      {.key = "port",       .type = ONION_INT_TYPE,    .ptr = &net_conf->port},
      {.key = "max_peers",  .type = ONION_INT_TYPE,    .ptr = &net_conf->max_peers},
      {.key = "max_queue",  .type = ONION_INT_TYPE,    .ptr = &net_conf->max_queue},

      // EPOLL
      {.key = "max_events_per_frame",  .type = ONION_INT_TYPE, .ptr = &epoll_conf->events_per_frame},
      {.key = "max_tag_queue_capable", .type = ONION_INT_TYPE, .ptr = &epoll_conf->tag_queue_capable},
      {.key = "peer_timeout",          .type = ONION_INT_TYPE, .ptr = &epoll_conf->timeout},
      {.key = "timer_sec_interval",    .type = ONION_INT_TYPE, .ptr = &epoll_conf->sec_interval},
      {.key = "timer_nsec_interval",   .type = ONION_INT_TYPE, .ptr = &epoll_conf->nsec_interval},
      {.key = "timer_sec_value",       .type = ONION_INT_TYPE, .ptr = &epoll_conf->sec_value},
      {.key = "timer_nsec_value",      .type = ONION_INT_TYPE, .ptr = &epoll_conf->nsec_value},
   };
   size_t entry_size = sizeof(entry) / sizeof(entry[0]);
   qsort(entry, entry_size, sizeof(entry[0]), onion_config_cmp);

   onion_read_ini(config_path, entry, entry_size, sizeof(entry[0]));

   ret = onion_core_conf_init(core_conf);
   if (ret < 0) {
      DEBUG_ERR("Onion core configuration initialization failed.\n");
      goto free_conf;
   }

   ret = onion_epoll_conf_init(epoll_conf);
   if (ret < 0) {
      DEBUG_ERR("Onion epoll configuration initialization failed.\n");
      goto free_conf;
   }

   ret = onion_net_conf_init(net_conf);
   if (ret < 0) {
      DEBUG_ERR("Onion net configuration initialization failed.\n");
      goto free_conf;
   }


DEBUG_INFO( "\n"
    "Core configuration:    count = %-3d,  sched = %d;\n"
    "Network configuration: IP    = %-15s, port  = %-5d, max_peers = %-5d, max_queue = %d;\n"
    "Epoll configuration:   events_per_frame = %-3d, tag_queue_capable = %-3d, timeout = %-3d, "
    "sec_interval = %-3d, nsec_interval = %-3d, sec_value = %-3d, nsec_value = %d;\n",
    core_conf->count, core_conf->sched,
    net_conf->ip_address, net_conf->port, net_conf->max_peers, net_conf->max_queue,
    epoll_conf->events_per_frame, epoll_conf->tag_queue_capable, epoll_conf->timeout,
    epoll_conf->sec_interval, epoll_conf->nsec_interval, epoll_conf->sec_value, epoll_conf->nsec_value
);




   return config;
free_conf:

unsucccessfull:
   return NULL;
}
