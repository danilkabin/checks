#include "account.h"
#include "binsocket.h"
#include "listhead.h"
#include "utils.h"
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>
#include <urcu/urcu-memb.h>

thread_m authorization_core;

void bs_authorization_user(sockType client_fd, struct binSocket_client *anonymous) {
   DEBUG_FUNC("hello new client!\n");
} 

void bs_user_accept(sockType client_fd, struct binSocket_client *anonymous) {
   bs_authorization_user(client_fd, anonymous);
}

void *bs_anonymous_runservice(void *arg) {
   struct binSocket_client *client;
   while (!anonymous_core.isShouldStop) {
      rcu_read_lock();
      list_for_each_entry_rcu(client, &bs_anonymous_list, list) {
        bs_recv_message(client, sizeof(client->buff), MSG_DONTWAIT); 
      }
      rcu_read_unlock();   
      usleep(100000);
   }
   return NULL;
}

int authorisation_init(void) {

   return 0;
}

void authorisation_exit(void) {

}
