#ifndef UIDQ_LOG_INCLUDE_H
#define UIDQ_LOG_INCLUDE_H

#include <stdio.h>

#define UIDQ_COLOR_RESET  "\x1b[0m"
#define UIDQ_COLOR_YELLOW "\x1b[33m"
#define UIDQ_COLOR_RED    "\x1b[31m"

typedef struct {
   char *debug;
   char *warn;
   char *err;
} uidq_log_conf_t;

typedef struct {
   int initialized;

   FILE *debug;
   FILE *warn;
   FILE *err;

   uidq_log_conf_t conf;
} uidq_log_t;

int uidq_log_isvalid(uidq_log_t *log);
uidq_log_conf_t *uidq_log_conf_get(uidq_log_t *log);

uidq_log_t *uidq_log_create(uidq_log_conf_t *conf);
void        uidq_log_abort(uidq_log_t *log);

int  uidq_log_init(uidq_log_t *log, uidq_log_conf_t *conf); 
void uidq_log_exit(uidq_log_t *log);

void uidq_debug_inline(uidq_log_t *log, char *format, ...);
void uidq_debug(uidq_log_t *log, char *format, ...);
void uidq_warn(uidq_log_t *log, char *format, ...);
void uidq_err(uidq_log_t *log, char *format, ...);

#endif /* UIDQ_LOG_INCLUDE_H */
