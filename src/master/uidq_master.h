#ifndef UIDQ_MASTER_H
#define UIDQ_MASTER_H

#include <sys/types.h>

pid_t uidq_fork_init();
void uidq_fork_exit(pid_t fork_pid);

int uidq_master_init();
void uidq_master_exit();

#endif
