#ifndef UIDQ_FORK_H
#define UIDQ_FORK_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
   pid_t master_pid;
   int status;

   int unix_fd;
} uidq_proc_t;

#endif
