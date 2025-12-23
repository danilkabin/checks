#ifndef OPIUM_LOG_INCLUDE_H
#define OPIUM_LOG_INCLUDE_H

#include "core/opium_core.h"

#define OPIUM_COLOR_RESET  "\x1b[0m"
#define OPIUM_COLOR_YELLOW "\x1b[33m"
#define OPIUM_COLOR_RED    "\x1b[31m"

struct opium_log_s {
   int initialized;

   FILE *debug;
   FILE *warn;
   FILE *err;
};

int opium_log_isvalid(opium_log_t *log);

opium_log_t *opium_log_init(char *debug, char *warn, char *err);
void opium_log_exit(opium_log_t *log);

void opium_log_debug_inline(opium_log_t *log, char *format, ...);
void opium_log_debug(opium_log_t *log, char *format, ...);
void opium_log_warn(opium_log_t *log, char *format, ...);
void opium_log_err(opium_log_t *log, char *format, ...);

void opium_log_stdout(char *format, ...);
void opium_log_stderr(char *format, ...);


#endif /* OPIUM_LOG_INCLUDE_H */

