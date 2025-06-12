#include "utils.h"
#include "binsocket.h"
#include "receive.h"
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

size_t peer_sock_recv(struct server_sock *bs, struct peer_sock *peer, int flags, size_t buff_size) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs/peer\n");
      return (size_t)-1;
   }
   if (peer->buff_len >= BS_CLIENT_BUFF_SIZE) {
      DEBUG_FUNC("max buff size\n");
      return (size_t)-1;
   }
   ssize_t bytes_read = recv(peer->sock.fd, peer->buff + peer->buff_len, buff_size - peer->buff_len, flags);
   if (bytes_read > 0) {
      printf("%s\n", peer->buff);
      peer->buff_len += bytes_read;
      peer->sock.packets_received++;
      // DEBUG_FUNC("bytes read: %zu from: %d\n", bytes_read, peer->sock.fd);
   } else if (bytes_read == 0) {
      // DEBUG_FUNC("peer disconnected: %d\n", peer->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}
