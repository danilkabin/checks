#include "uidq_log.h"
#include "uidq_alloc.h"
#include "uidq_utils.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

uidq_log_t *uidq_logger;

void uidq_log_msg(FILE *file, const char *color_code, const char *prefix, const char *format, va_list args) {
   if (color_code && (file == stderr || file == stdout)) {
      fprintf(file, "%s", color_code);
   }
   if (prefix) {
      fprintf(file, "%s: ", prefix);
   }
   vfprintf(file, format, args);
   if (color_code && (file == stderr || file == stdout)) {
      fprintf(file, "\x1b[0m");
   }
   fflush(file);
}

void uidq_debug(uidq_log_t *uidq_log, char *format, ...) {
   if (!uidq_log) return;
   FILE *debug = uidq_log->debug ? uidq_log->debug : stdout;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(debug, UIDQ_COLOR_RESET, "[DEBUG]", format, args); 
   va_end(args);
}

void uidq_warn(uidq_log_t *uidq_log, char *format, ...) {
   if (!uidq_log) return;
   FILE *warn = uidq_log->warn ? uidq_log->warn : stderr;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(warn, UIDQ_COLOR_RESET, "[WARN]", format, args); 
   va_end(args);
}

void uidq_err(uidq_log_t *uidq_log, char *format, ...) {
   if (!uidq_log) return;
   FILE *err = uidq_log->err ? uidq_log->err : stderr;

   va_list args;
   va_start(args, format); 
   uidq_log_msg(err, UIDQ_COLOR_RESET, "[ERROR]", format, args); 
   va_end(args);
}

uidq_log_t *uidq_log_init(char *debug, char *warn, char *err) {
   uidq_log_t *uidq_log = uidq_calloc(sizeof(uidq_log_t)); 
   if (!uidq_log) {
      DEBUG_ERR("Failed to allocate uidq log.\n");
      return NULL;
   }

   uidq_log->debug = fopen(debug, "a");
   if (!uidq_log->debug) {
      uidq_log->debug = NULL;
   } 

   uidq_log->warn = fopen(warn, "a");
   if (!uidq_log->warn) {
      uidq_log->warn = NULL;
   } 

   uidq_log->err = fopen(err, "a");
   if (!uidq_log->err) {
      uidq_log->err = NULL;
   }

   uidq_logger = uidq_logger == NULL ? uidq_log : uidq_logger; 

   return uidq_log;
}

void uidq_log_exit(uidq_log_t *uidq_log) {
   if (uidq_log->debug) {
      fclose(uidq_log->debug);
      uidq_log->debug = NULL;
   }
   if (uidq_log->warn) {
      fclose(uidq_log->warn);
      uidq_log->warn = NULL;
   }
   if (uidq_log->err) {
      fclose(uidq_log->err);
      uidq_log->err = NULL;
   }
   if (uidq_log == uidq_logger) {
      uidq_logger = NULL;
   }
   free(uidq_log);
}
