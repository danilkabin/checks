#define _GNU_SOURCE

#include <sys/socket.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/uidq_alloc.h"
#include "core/uidq_slab.h"
#include "core/uidq_utils.h"
#include "core/uidq_log.h"
#include "master/uidq_master_proc.h"

int uidq_affinity_count(void) {
   long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
   return (nprocs < 1) ? 1 : (int)nprocs;
}

int uidq_affinity_get(void) {
   int cpu = sched_getcpu();
   if (cpu < 0) {
      UIDQ_ERR("Failed to get CPU: %s\n", strerror(errno));
      return -1;
   }
   return cpu;
}

int uidq_affinity_set(int core) {
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core, &cpuset);

   pid_t pid = 0;

   if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset)) {
      int current_core = uidq_affinity_get();
      UIDQ_ERR("Failed to set CPU affinity: current core=%d, target core=%d\n", current_core, core);
      return -1;
   }

   return 0;
}

static void uidq_proc_exec(void *data) {
   if (!data) {
      UIDQ_ERR("Invalid exec data\n");
      exit(EXIT_FAILURE);
   }

   uidq_exec_var_t *exec = (uidq_exec_var_t *)data;
   if (!exec->path || !exec->argv || !exec->envp) {
      UIDQ_ERR("Invalid exec parameters: path=%p, argv=%p, envp=%p\n",
            (void *)exec->path, (void *)exec->argv, (void *)exec->envp);
      exit(EXIT_FAILURE);
   }

   if (execve(exec->path, exec->argv, exec->envp) == -1) {
      UIDQ_ERR("Failed to execve '%s' (%s): %s\n",
            exec->name, exec->path, strerror(errno));
      exit(EXIT_FAILURE);
   }
}

pid_t uidq_process_create(void *data) {
   pid_t fork_pid = fork();

   switch (fork_pid) {
      case -1:
         UIDQ_ERR("Failed to create process.\n");
         break;
      case 0:
         DEBUG_FUNC("FSDFSDFSD FSDNFSD FSD F SDF SD\n");
         uidq_proc_exec(data);
         _exit(1);
         break;
      default:
         UIDQ_ERR("Failed to fork().\n");
         break;
   }

   DEBUG_FUNC("PID: %ld, : %s fork_pid: %ld\n",
         (long)getpid(), (fork_pid == 0) ? "(child) " : "(parent) ", (long)fork_pid);

   return fork_pid;
}

uidq_process_t *uidq_process_init(uidq_process_ctl_t *ctl) {
   uidq_slab_t *processes = ctl->processes;

   int pos = uidq_slab_alloc(processes, NULL, sizeof(uidq_process_t));
   if (pos < 0) {
      UIDQ_ERR("Failed to allocate process.\n");
      return NULL;
   }

   uidq_process_t *process = uidq_slab_get(processes, pos);
   if (!process) {
      UIDQ_ERR("Slab returned NULL after allocation.\n");
      return NULL;
   }

   memset(process, 0, sizeof(uidq_process_t));
   process->index = pos;   

   char pid_str[16];
   sprintf(pid_str, "%d", (int)getpid());

   char *path = UIDQ_PROCESS_WORKER_PATH;
   char *name = "process_worker";
   char *const argv[] = { name, pid_str, NULL };
   char *const envp[] = { NULL };

   uidq_exec_var_t exec = {
      .path = path,
      .name = name,
      .argv = argv,
      .envp = envp 
   }; 

   process->pid = uidq_process_create(&exec);
   if (process->pid <= 0) {
      UIDQ_ERR("Failed to allocate uidq process.\n");
      goto fail;
   } 

   ctl->count = ctl->count + 1;
while (1) {

}
   return process;

fail:
   uidq_process_exit(ctl, process);
   return NULL;
}

void uidq_process_exit(uidq_process_ctl_t *ctl, uidq_process_t *process) {

}

uidq_process_ctl_t *uidq_process_ctl_init(int max_count) {
   if (max_count < 1) {
      DEBUG_ERR("Invalid parameter max_count is less than 1.\n");
      return NULL;
   }

   uidq_process_ctl_t *ctl = uidq_calloc(sizeof(uidq_process_ctl_t));
   if (!ctl) {
      UIDQ_ERR("Failed to allocate process controller.\n");
      return NULL;
   }

   ctl->processes = uidq_slab_create_and_init(max_count, sizeof(uidq_process_t));
   if (!ctl->processes) {
      UIDQ_ERR("Failed to allocate processes.\n");
      goto fail;
   }

   ctl->count = 0;
   ctl->max_count = max_count;

   return ctl;

fail:
   uidq_process_ctl_exit(ctl);
   return NULL;
}

void uidq_process_ctl_exit(uidq_process_ctl_t *ctl) {
   free(ctl);
}
