#include "core/opium_core.h"

   int
opium_log_isvalid(opium_log_t *log)
{
   return log && log->initialized == 1;
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

   void 
opium_log_debug_inline(opium_log_t *log, char *format, ...) 
{
   if (!log) return;
   FILE *debug = log->debug ? log->debug : stdout;

   va_list args;
   va_start(args, format);
   vfprintf(debug, format, args);
   va_end(args);

   fflush(debug);
}

   void 
opium_log_debug(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *debug = log->debug ? log->debug : stdout;

   va_list args;
   va_start(args, format); 
   opium_log_msg(debug, OPIUM_COLOR_RESET, "[DEBUG]", format, args);
   va_end(args);
}

   void 
opium_log_warn(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *warn = log->warn ? log->warn : stderr;

   va_list args;
   va_start(args, format); 
   opium_log_msg(warn, OPIUM_COLOR_RESET, "[WARN]", format, args);
   va_end(args);
}

   void 
opium_log_err(opium_log_t *log, char *format, ...)
{
   if (!log) return;
   FILE *err = log->err ? log->err : stderr;

   va_list args;
   va_start(args, format); 
   opium_log_msg(err, OPIUM_COLOR_RESET, "[ERROR]", format, args);
   va_end(args);
}

void
opium_log_stdout(char *format, ...)
{
    va_list args;
    va_start(args, format);
    opium_log_msg(stdout, OPIUM_COLOR_RESET, "[INFO]", format, args);
    va_end(args);
}

void
opium_log_stderr(char *format, ...)
{
    va_list args;
    va_start(args, format);
    opium_log_msg(stderr, OPIUM_COLOR_RESET, "[ERROR]", format, args);
    va_end(args);
}

   opium_log_t * 
opium_log_init(char *debug, char *warn, char *err)
{

   opium_log_t *log = calloc(1, sizeof(opium_log_t));
   if (!log) {
      opium_log_err(log, "Failed to allocate hash table.\n");
      return NULL;
   }
   log->initialized = 1;

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

   return log;
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

   free(log);
}

