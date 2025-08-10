#include "uidq_conf_file.h"
#include "core/uidq_alloc.h"
#include "core/uidq_conf_parser.h"
#include "core/uidq_types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int uidq_conf_isvalid(uidq_conf_t *conf) {
   if (!conf || !conf->file || !conf->log) return UIDQ_ERROR;
   return UIDQ_OK;
}

void uidq_conf_set_pos(uidq_conf_t *conf, int pos) {
   if (uidq_conf_isvalid(conf) != UIDQ_OK) return;
   conf->pos = pos;
   fseek(conf->file, conf->pos, SEEK_SET);
}

char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

uidq_conf_t *uidq_conf_init(const char *filename, uidq_log_t *log) {
   if (!log) {
      return NULL;
   }

   uidq_conf_t *conf = uidq_calloc(sizeof(uidq_conf_t));
   if (!conf) {
      uidq_err(log, "Failed to allocate config struct.\n");
      return NULL;
   }

   conf->file = fopen(filename, "r");
   if (!conf->file) {
      uidq_err(log, "Failed to open file.\n");
      goto fail;
   }

   const char *text = read_file(filename);
   uidq_pidr_t *pidr = uidq_pidr_init(text, log);

   conf->pos = 0;
   conf->log = log;

   fseek(conf->file, conf->pos, SEEK_SET);

   return conf;
fail:
   uidq_conf_free(conf);
   return NULL;
}

void uidq_conf_free(uidq_conf_t *conf) {
   if (!conf) return;

   if (conf->file) {
      fclose(conf->file);
      conf->file = NULL;
   }

   conf->pos = 0;

   free(conf);
}
