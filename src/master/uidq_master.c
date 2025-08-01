#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "uidq_master.h"
#include "uidq_master_process.h"
#include "uidq_utils.h"

int uidq_master_init() {
   uidq_proc_ctl_t *proc_ctl = uidq_proc_ctl_init(6);
   if (!proc_ctl) {
      DEBUG_ERR("Failed to initialize proc ctl.\n");
      return -1;
   }
 
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

   uidq_proc_t *proc = uidq_proc_ctl_add(proc_ctl, &exec);
   if (!proc) {
      DEBUG_ERR("dsfsdf\n");
      return -1;
   }

   DEBUG_FUNC("PROC ID!!: %d\n", proc->pid);
   
   return 0;
}

void uidq_master_exit() {

}
