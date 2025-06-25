#include "onion.h"
#include <onion/onion.h>
#include <stdio.h>

int main() {
   onion_config_t onion_config;
   onion_config_init(&onion_config);
   printf("line method max size:%zu\n", onion_config.http_line_method_max_size);
   return 0;
}
