#ifndef UIDQ_CONF_FILE_H
#define UIDQ_CONF_FILE_H

#include "core/uidq_core.h"
#include "core/uidq_log.h"
#include <stdio.h>

typedef struct {
   FILE *file;
   char *name;
   int pos;

   uidq_log_t *log;
} uidq_conf_t;

// API
uidq_conf_t *uidq_conf_init(const char *filename, uidq_log_t *log);
void uidq_conf_free(uidq_conf_t *conf);

#endif
