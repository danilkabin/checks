#include "account.h"
#include "binsocket.h"
#include "utils.h"
#include <sys/socket.h>

thread_m authorization_core;

void bs_authorization_user(sockType client_fd, struct binSocket_client *anonymous) {
   DEBUG_FUNC("hello new client!\n");

} 

void bs_user_accept(sockType client_fd, struct binSocket_client *anonymous) {
   bs_authorization_user(client_fd, anonymous);
}

int authorisation_init(void) {

   return 0;
}

void authorisation_exit(void) {

}
