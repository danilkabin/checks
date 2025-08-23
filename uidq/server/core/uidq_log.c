#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "uidq_log.h"

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

void
uidq_log_init(uidq_log_t *log, char *debug, char *warn, char *err) 
{
   log->debug = fopen(debug, "a");
   if (!log->debug) {
      log->debug = NULL;
   } 

   log->warn = fopen(warn, "a");
   if (!log->warn) {
      log->warn = NULL;
   } 

   log->err = fopen(err, "a");
   if (!log->err) {
      log->err = NULL;
   }
}

void 
uidq_log_exit(uidq_log_t *log) 
{
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
