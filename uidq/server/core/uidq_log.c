#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "uidq_log.h"
#include "core/uidq_alloc.h"
#include "core/uidq_core.h"

   int
uidq_log_isvalid(uidq_log_t *log) 
{
   return log && log->initialized == 1;
}

uidq_log_conf_t *
uidq_log_conf_get(uidq_log_t *log) {
   if (!uidq_log_isvalid(log)) return NULL;
   return &log->conf;
}

   void
uidq_log_msg(FILE *file, const char *cc, const char *pref, const char *format, va_list args)
{
   if (cc && (file == stderr || file == stdout)) {
      fprintf(file, "%s", cc);
   }
   if (pref) {
      fprintf(file, "%s: ", pref);
   }
   vfprintf(file, format, args);
   if (cc && (file == stderr || file == stdout)) {
      fprintf(file, "\x1b[0m");
   }
   fflush(file);
}

void uidq_debug_inline(uidq_log_t *log, char *format, ...) {
    if (!log) return;
    FILE *debug = log->debug ? log->debug : stdout;

    va_list args;
    va_start(args, format);
    vfprintf(debug, format, args);
    va_end(args);

    fflush(debug);
}

   void 
uidq_debug(uidq_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *debug = log->debug ? log->debug : stdout;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(debug, UIDQ_COLOR_RESET, "[DEBUG]", format, args); 
   va_end(args);
}

   void 
uidq_warn(uidq_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *warn = log->warn ? log->warn : stderr;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(warn, UIDQ_COLOR_RESET, "[WARN]", format, args); 
   va_end(args);
}

   void 
uidq_err(uidq_log_t *log, char *format, ...) 
{
   if (!log) return;
   FILE *err = log->err ? log->err : stderr;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(err, UIDQ_COLOR_RESET, "[ERROR]", format, args); 
   va_end(args);
}

   uidq_log_t *
uidq_log_create(uidq_log_conf_t *conf)
{
   uidq_log_t *log = uidq_calloc(sizeof(uidq_log_t), NULL);
   if (!log) {
      uidq_err(log, "Failed to allocate hash table.\n"); 
      return NULL;
   }
   log->initialized = 1;

   if (uidq_log_init(log, conf) != UIDQ_RET_OK) {
      uidq_err(log, "Failed to init log table.\n");
      uidq_log_abort(log);
      return NULL;
   }

   return log;
}

   void
uidq_log_abort(uidq_log_t *log)
{
   if (!uidq_log_isvalid(log)) return;

   uidq_log_exit(log);
   log->initialized = 0;
   uidq_free(log, NULL);
}

   int
uidq_log_init(uidq_log_t *log, uidq_log_conf_t *conf) 
{
   log->debug = fopen(conf->debug, "a");
   if (!log->debug) {
      log->debug = NULL;
   } 

   log->warn = fopen(conf->warn, "a");
   if (!log->warn) {
      log->warn = NULL;
   } 

   log->err = fopen(conf->err, "a");
   if (!log->err) {
      log->err = NULL;
   }

   return UIDQ_RET_OK;
}

   void 
uidq_log_exit(uidq_log_t *log) 
{
   if (!uidq_log_isvalid(log)) return;

   if (log->debug) {
      fclose(log->debug);
      log->debug = NULL;
   }
   if (log->warn) {
      fclose(log->warn);
      log->warn = NULL;
   }
   if (log->err) {
      fclose(log->err);
      log->err = NULL;
   }
}
