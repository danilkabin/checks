#include <sys/types.h>
#include <unistd.h>

#include "uidq_master.h"
#include "uidq_utils.h"

pid_t uidq_fork_init() {
   int stack = 222;
   pid_t fork_pid;

   switch (fork_pid = fork()) {
      case -1:

         break;
      case 0:
         execl("/home/dima/Projects/testix/src/process/", "ls", "-l", NULL);
         break;
      default:
         break;
   }

   DEBUG_FUNC("PID: %ld %s\n",
         (long)getpid(), (fork_pid == 0) ? "(child) " : "(parent) ");

   return fork_pid;
}

void uidq_fork_exit(pid_t fork_pid) {

}

int uidq_master_init() {

   return 0;
}

void uidq_master_exit() {

}
