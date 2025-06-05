#include "utils.h"
#include "main.h"
#include "device.h"
#include "peer.h"
#include "socket.h"
#include "binsocket.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
   struct tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(51234),
      .addr.s_addr = htonl(INADDR_ANY)
   };

   fb_peers_init();
   sock_syst_init();
   struct server_sock *sock = server_sock_create(&port_conf);
   if (!sock) {
      DEBUG_INFO("where my sock!!\n");
      return -1;
   }

   int ret = server_sock_init(sock);
   if (ret < 0) {
      DEBUG_INFO("where my init sock!!\n");
      return -1;
   }
 /*  for (int i = 0; i < 5; i++) {
      accept_callback_sk func = bs_user_accept;
      int ret = binSocket_accept(sock, func);
   sleep(1);
   }*/
   sleep(10);
   sock_syst_exit(sock);
   DEBUG_FUNC("heehllo1oo1o1\n");
   return 0;
}
