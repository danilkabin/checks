#include "utils.h"
#include "send.h"
#include "device.h"

#include <stddef.h>
#include <sys/socket.h>
#include <string.h> 
#include <errno.h>

const char *response =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/plain\r\n"
"Content-Length: 6\r\n"   // длина "Hello\n" — 6 байт
"\r\n"
"Hello\n";

size_t sentt = 0;

/*size_t onion_peer_sock_send(struct onion_server_sock *bs, struct onion_peer_sock *peer, int flags) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs peer\n");
      return (size_t)-1;
   }

   return 0;
}*/
