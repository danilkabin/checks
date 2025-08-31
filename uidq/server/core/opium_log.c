#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "opium_log.h"
#include "core/opium_alloc.h"
#include "core/opium_core.h"

   int
opium_log_isvalid(opium_log_t *log)
{
   return log && log->initialized == 1;
}

opium_log_conf_t *
opium_log_conf_get(opium_log_t *log) {
   if (!opium_log_isvalid(log)) return NULL;
   return &log->conf;
}

   void
opium_log_msg(FILE *file, const char *cc, const char *pref, const char *format, va_list args)
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

void opium_debug_inline(opium_log_t *log, char *format, ...) {
    if (!log) return;
    FILE *debug = log->debug ? log->debug : stdout;

    va_list args;
    va_start(args, format);
    vfprintf(debug, format, args);
    va_end(args);

    fflush(debug);
}

   void 
opium_debug(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *debug = log->debug ? log->debug : stdout;

   va_list args;
   va_start(args, format); 
   opium_log_msg(debug, OPIUM_COLOR_RESET, "[DEBUG]", format, args);
   va_end(args);
}

   void 
opium_warn(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *warn = log->warn ? log->warn : stderr;

   va_list args;
   va_start(args, format); 
   opium_log_msg(warn, OPIUM_COLOR_RESET, "[WARN]", format, args);
   va_end(args);
}

   void 
opium_err(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *err = log->err ? log->err : stderr;

   va_list args;
   va_start(args, format); 
   opium_log_msg(err, OPIUM_COLOR_RESET, "[ERROR]", format, args);
   va_end(args);
}

   opium_log_t *
opium_log_create(opium_log_conf_t *conf)
{
   opium_log_t *log = opium_calloc(sizeof(opium_log_t), NULL);
   if (!log) {
      opium_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }
   log->initialized = 1;

   if (opium_log_init(log, conf) != OPIUM_RET_OK) {
      opium_err(log, "Failed to init log table.\n");
      opium_log_abort(log);
      return NULL;
   }

   return log;
}

   void
opium_log_abort(opium_log_t *log)
{
   if (!opium_log_isvalid(log)) return;

   opium_log_exit(log);
   log->initialized = 0;
   opium_free(log, NULL);
}

   int
opium_log_init(opium_log_t *log, opium_log_conf_t *conf)
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

   return OPIUM_RET_OK;
}

   void 
opium_log_exit(opium_log_t *log)
{
   if (!opium_log_isvalid(log)) return;

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
