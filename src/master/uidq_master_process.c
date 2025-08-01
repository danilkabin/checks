#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "uidq_master_process.h"
#include "uidq_slab.h"
#include "uidq_utils.h"

int uidq_get_affinity() {

   return 0;
}

int uidq_set_affinity() {

   return 0;
}

bool uidq_proc_ctl_isvalid(uidq_proc_ctl_t *ctl) {
   if (!ctl || !ctl->initialized) {
      return false;
   }
   return true;
}

static void uidq_proc_exec(void *data) {
   uidq_exec_var_t *exec = data;
   if (execve(exec->path, exec->argv, exec->envp) == -1) {
      DEBUG_ERR("Failed to execve(). name: %s, path: %s\n",
            exec->name, exec->path);
   }
   exit(1);
}

pid_t uidq_process_create(void *data) {
   pid_t fork_pid = fork(); 
   
   switch (fork_pid) {
      case -1:
         DEBUG_ERR("Failed to create process.\n");
         break;
      case 0:
         DEBUG_FUNC("fsdfsd\n");
         uidq_proc_exec(data);
         _exit(1);
      break;
      default:
         DEBUG_ERR("Failed to fork().\n");
         break;
   }

   DEBUG_FUNC("PID: %ld, : %s fork_pid: %ld\n",
         (long)getpid(), (fork_pid == 0) ? "(child) " : "(parent) ", (long)fork_pid);

   return fork_pid;
}

void uidq_process_remove(pid_t fork_pid) {

}

uidq_proc_t *uidq_proc_ctl_add(uidq_proc_ctl_t *ctl, void *data) {
   if (!uidq_proc_ctl_isvalid(ctl)) {
      DEBUG_ERR("Controller is invalid.\n");
      goto fail;
   }

   if (ctl->count >= ctl->max_count) {
      DEBUG_ERR("Process count exceeded: count=%u, max_count=%u\n",
            ctl->count, ctl->max_count);
      goto fail;
   }

   uidq_slab_t *procs = ctl->processes;

   pid_t pid = uidq_process_create(data);
   if (pid < 0) {
      DEBUG_ERR("Failed to allocate pid.\n");
      goto fail;
   }

   size_t proc_size = sizeof(uidq_proc_t);
   int pos = uidq_slab_alloc(procs, NULL, proc_size);
   if (pos < 0) {
      DEBUG_ERR("Failed to allocate proc into slab.\n");
      goto process_exit;
   }

   uidq_proc_t *proc = uidq_slab_get(ctl->processes, pos);
   proc->pid = pid;
   proc->pos = pos;

   ctl->count = ctl->count + 1;

   return proc;
process_exit:
   kill(pid, SIGTERM);
   waitpid(pid, NULL, 0);
fail:
   return NULL;
}

void uidq_proc_ctl_del(uidq_proc_ctl_t *ctl, uidq_proc_t *proc) {
   if (!proc) {
      return;
   }

   pid_t pid = proc->pid;
   int pos = proc->pos;

   kill(pid, SIGTERM);
   uidq_slab_dealloc(ctl->processes, pos);
}

uidq_proc_ctl_t *uidq_proc_ctl_init(uint32_t max_count) {
   if (max_count == 0) {
      DEBUG_ERR("Invalid data.\n");
      goto fail;
   }

   size_t ctl_size = sizeof(uidq_proc_ctl_t);
   uidq_proc_ctl_t *ctl = malloc(ctl_size);
   if (!ctl) {
      DEBUG_ERR("Failed to allocate controller.\n");
      goto fail;
   }

   size_t block_size = sizeof(uidq_proc_t);
   size_t max_size = max_count * block_size; 
   ctl->processes = uidq_slab_create_and_init(max_size, block_size); 
   if (!ctl->processes) {\
      DEBUG_ERR("Failed to allocate ctl processes.\n");
      goto ctl_exit;
   }

   ctl->count = 0;
   ctl->max_count = max_count;
   ctl->initialized = true;

   return ctl;

ctl_exit:
   uidq_proc_ctl_exit(ctl);
fail:
   return NULL;
}

void uidq_proc_ctl_exit(uidq_proc_ctl_t *ctl) {
   if (!ctl) {
      return;
   }

   if (ctl->processes) {
      uidq_slab_exit(ctl->processes);
      ctl->processes = NULL;
   }

   ctl->count = 0;
   ctl->max_count = 0;
   ctl->initialized = false;

   free(ctl);
}
