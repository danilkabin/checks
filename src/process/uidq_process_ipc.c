#include <string.h>

#include "core/uidq_socket.h"
#include "process/uidq_process_ipc.h"
#include "process/uidq_config.h"

int uidq_proc_ipc_init() {
   uidq_sock_conf_t conf;
   memset(&conf, 0, sizeof(conf));
   
   conf.domain = AF_UNIX;
   conf.type = SOCK_STREAM;
   conf.protocol = 0;
   conf.queue_capable = uidq_proc_conf.max_queue;

   memcpy(conf.sock_un.sun_path, "/tmp/uidqsock.sock", sizeof(conf.sock_un.sun_path) + 1);

   return uidq_sock_init(&conf);
}

void uidq_proc_ipc_exit() {

}
