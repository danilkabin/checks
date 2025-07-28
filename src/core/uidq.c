#include "uidq.h"
#include "../master/uidq_master.h"
#include "misc/uidq_utils.h"
#include <stdint.h>

int uidq_module_init() {
   DEBUG_FUNC("Hello mom!\n");
   pid_t fork_pid = uidq_fork_init();
   return -1;
}

void uidq_module_exit() { }
