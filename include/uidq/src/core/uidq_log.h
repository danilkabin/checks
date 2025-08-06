#ifndef UIDQ_LOG_H
#define UIDQ_LOG_H

#include <stdio.h>

#define UIDQ_COLOR_RESET  "\x1b[0m"
#define UIDQ_COLOR_YELLOW "\x1b[33m"
#define UIDQ_COLOR_RED    "\x1b[31m"

typedef struct {
   FILE *debug;
   FILE *warn;
   FILE *err;
} uidq_log_t;

extern uidq_log_t *uidq_logger;

void uidq_debug(uidq_log_t *uidq_log, char *format, ...);
void uidq_warn(uidq_log_t *uidq_log, char *format, ...);
void uidq_err(uidq_log_t *uidq_log, char *format, ...);

uidq_log_t *uidq_log_init(char *debug, char *warn, char *err);
void uidq_log_exit(uidq_log_t *uidq_log);

#define UIDQ_DEBUG(fmt, ...) uidq_debug(uidq_logger, fmt, ##__VA_ARGS__)
#define UIDQ_WARN(fmt, ...)  uidq_warn(uidq_logger, fmt, ##__VA_ARGS__)
#define UIDQ_ERR(fmt, ...)   uidq_err(uidq_logger, fmt, ##__VA_ARGS__)

#endif
