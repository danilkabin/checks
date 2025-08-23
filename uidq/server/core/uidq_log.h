#ifndef UIDQ_LOG_INCLUDE_H
#define UIDQ_LOG_INCLUDE_H

#include <stdio.h>

#define UIDQ_COLOR_RESET  "\x1b[0m"
#define UIDQ_COLOR_YELLOW "\x1b[33m"
#define UIDQ_COLOR_RED    "\x1b[31m"

typedef struct {
   FILE *debug;
   FILE *warn;
   FILE *err;
} uidq_log_t;

void uidq_log_init(uidq_log_t *log, char *debug, char *warn, char *err); 
void uidq_log_exit(uidq_log_t *log);

void uidq_debug(uidq_log_t *log, char *format, ...);
void uidq_warn(uidq_log_t *log, char *format, ...);
void uidq_err(uidq_log_t *log, char *format, ...);

#endif /* UIDQ_LOG_INCLUDE_H */
