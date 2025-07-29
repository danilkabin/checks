#include "uidq.h"
#include "../master/uidq_master.h"
#include "misc/uidq_utils.h"
#include <stdint.h>
#include <unistd.h>

int uidq_module_init() {
   DEBUG_FUNC("Hello mom!\n");

   char pid_str[16];
   sprintf(pid_str, "%d", (int)getpid());

   char *path = UIDQ_PROCESS_WORKER_PATH;
   char *name = UIDQ_PROCESS_WORKER_NAME;
   char *const argv = pid_str;
   char *const envp = NULL;

   uidq_exec_var_t exec = {
      .path = path,
      .name = name,
      .argv = &argv,
      .envp = &envp 
   }; 

   pid_t fork_pid = uidq_process_create(&exec);
   while (1) {

   }
   return -1;
}

void uidq_module_exit() { }
