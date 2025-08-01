#include "uidq_master_config.h"
#include "uidq_utils.h"
#include <stdlib.h>

uidq_master_conf_t *uidq_master_conf_init(uidq_master_conf_arg_t *arg) {

   size_t conf_size = sizeof(uidq_master_conf_t);
   uidq_master_conf_t *conf = malloc(conf_size);
   if (!conf) {
      DEBUG_ERR("Failed to allocate config.\n");
      goto fail;
   }



   return conf; 
fail:
   return NULL;
}

void uidq_master_conf_exit(uidq_master_conf_t *conf) {

   free(conf);
}


