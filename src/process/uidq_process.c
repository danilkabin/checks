#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "core/uidq_utils.h"
#include "process/uidq_process.h"

int main(int argc, char *argv[]) {
   pid_t master_pid = (pid_t)atoi(argv[0]);
   DEBUG_FUNC("Hello!\n");
   
}
