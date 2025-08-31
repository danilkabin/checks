#ifndef OPIUM_LOG_INCLUDE_H
#define OPIUM_LOG_INCLUDE_H

#include <stdio.h>

#define OPIUM_COLOR_RESET  "\x1b[0m"
#define OPIUM_COLOR_YELLOW "\x1b[33m"
#define OPIUM_COLOR_RED    "\x1b[31m"

typedef struct {
   char *debug;
   char *warn;
   char *err;
} opium_log_conf_t;

typedef struct {
   int initialized;

   FILE *debug;
   FILE *warn;
   FILE *err;

   opium_log_conf_t conf;
} opium_log_t;

int opium_log_isvalid(opium_log_t *log);
opium_log_conf_t *opium_log_conf_get(opium_log_t *log);

opium_log_t *opium_log_create(opium_log_conf_t *conf);
void        opium_log_abort(opium_log_t *log);

int  opium_log_init(opium_log_t *log, opium_log_conf_t *conf);
void opium_log_exit(opium_log_t *log);

void opium_debug_inline(opium_log_t *log, char *format, ...);
void opium_debug(opium_log_t *log, char *format, ...);
void opium_warn(opium_log_t *log, char *format, ...);
void opium_err(opium_log_t *log, char *format, ...);

#endif /* OPIUM_LOG_INCLUDE_H */
