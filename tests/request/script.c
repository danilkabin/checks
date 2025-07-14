#include <onion/onion.h>
#include <onion/engine.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TOTAL_SOCKETS_PER_CORE 1

int main() {
   onion_config_t *onion_config = onion_config_init();
   struct onion_worker_head_t *head = onion_device_init(&onion_config->server_triad);
   while (!head) {
    head = onion_device_init(&onion_config->server_triad);
    sleep(1);  
   }
   sleep(100);

   return 0;
}
