#include "parser.h"
#include "slab.h"
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
  http_request_t *request = &peer->request;

      /*if (request. = BS_CLIENT_BUFF_CAPACITY) {
      DEBUG_FUNC("max buff size\n");
      return (size_t)-1;
   }*/


   char buff[8192];
   size_t buff_size1 = sizeof(buff);
   
   ssize_t bytes_read = recv(peer->sock.fd, buff, buff_size1, flags);
   if (bytes_read > 0) {
      if (BS_CLIENT_BUFF_CAPACITY - request->buff_len < (size_t)bytes_read) {
         DEBUG_FUNC("buffer busy!\n");
         return -1;
      }
      http_request_parse(request, buff, bytes_read);
      peer->sock.packets_received++;
  //    DEBUG_FUNC("bytes read: %zu buff len size: %zu from: %d\n", bytes_read, parser->buff_len, peer->sock.fd);
   } else if (bytes_read == 0) {
      // DEBUG_FUNC("peer disconnected: %d\n", peer->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}
