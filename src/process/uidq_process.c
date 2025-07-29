#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "uidq_process.h"
#include "uidq_utils.h"

int main(int argc, char *argv[]) {
   pid_t master_pid = (pid_t)atoi(argv[0]);
   DEBUG_FUNC("Hello!\n");
   
   while (1) { 
      if (kill(master_pid, 0) < 0) {
         exit(0);
      }
      DEBUG_FUNC("pid: %ld\n", (long)getpid()); 
      sleep(1);
   }
}
