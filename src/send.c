#include "utils.h"
#include "send.h"
#include "binsocket.h"

#include <stddef.h>
#include <sys/socket.h>
#include <string.h> 
#include <errno.h>

size_t peer_sock_send(struct server_sock *bs, struct peer_sock *peer, int flags) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs peer\n");
      return (size_t)-1;
   }
const char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "\r\n"
    "Hello [afasf asfja sjf asjpidor";
       size_t request_len = strlen(response);
   ssize_t bytes_sent = send(peer->sock.fd, response, request_len, flags);
   if (bytes_sent > 0) {
      if ((size_t)bytes_sent < peer->buff_len) {
         memmove(peer->buff, peer->buff + bytes_sent, peer->buff_len - bytes_sent);
      }

      peer->buff_len -= bytes_sent;
       DEBUG_FUNC("bytes sent: %zd from: %d\n", bytes_sent, peer->sock.fd);
   } else if (bytes_sent == -1) {
      DEBUG_FUNC("send failed: %s\n", strerror(errno));
   }
   return bytes_sent;
}
