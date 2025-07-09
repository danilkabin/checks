#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

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
      DEBUG_FUNC("name: %s value: %d\n", entry->key, *(int*)entry->ptr);
   } else if (entry->type == ONION_STRING_TYPE) {
      strncpy((char*)entry->ptr, data, ONION_ENTRY_VALUE_SIZE);
      DEBUG_FUNC("name: %s value: %s\n", entry->key, (char*)entry->ptr);
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

   onion_core_conf_t *core_conf = &config->core;
   onion_net_conf_t *net_conf = &config->net;
   onion_http_conf_t *http_conf = &config->http;

   onion_conf_entry_t entry[] = {
      {.key = "core_count", .type = ONION_INT_TYPE, .ptr = &core_conf->count},
      {.key = "core_sched", .type = ONION_INT_TYPE, .ptr = &core_conf->sched},
      {.key = "worker_count", .type = ONION_INT_TYPE, .ptr = &core_conf->worker_count},

      {.key = "ip_address", .type = ONION_STRING_TYPE, .ptr = &net_conf->ip_address},
      {.key = "port", .type = ONION_INT_TYPE, .ptr = &net_conf->port},
      {.key = "max_peers", .type = ONION_INT_TYPE, .ptr = &net_conf->max_peers},
      {.key = "max_queue", .type = ONION_INT_TYPE, .ptr = &net_conf->max_queue},
      {.key = "peer_timeout", .type = ONION_INT_TYPE, .ptr = &net_conf->timeout},
   };
   size_t entry_size = sizeof(entry) / sizeof(entry[0]);
   qsort(entry, entry_size, sizeof(entry[0]), onion_config_cmp);

   onion_read_ini(config_path, entry, entry_size, sizeof(entry[0]));

   ret = onion_core_conf_init(core_conf);
   if (ret < 0) {
      DEBUG_ERR("Onion core configuration initialization failed.\n");
      goto free_conf;
   }
   
   ret = onion_net_conf_init(net_conf);
   if (ret < 0) {
      DEBUG_ERR("Onion net configuration initialization failed.\n");
      goto free_conf;
   }

   DEBUG_FUNC("core_count: %d, core_sched: %d\n", core_conf->count, core_conf->sched);
   DEBUG_FUNC("ip_address: %s, port: %d, max_peers: %d, max_queue: %d, timeout: %d\n", 
         net_conf->ip_address, net_conf->port, net_conf->max_peers, net_conf->max_queue, net_conf->timeout);

   return config;
free_conf:

unsucccessfull:
   return NULL;
}
