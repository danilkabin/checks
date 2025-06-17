#include "http.h"
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
   http_parser *parser = &peer->parser;
   
      if (parser->buff_len >= BS_CLIENT_BUFF_CAPACITY) {
      DEBUG_FUNC("max buff size\n");
      return (size_t)-1;
   }

   ssize_t bytes_read = recv(peer->sock.fd, parser->buff + parser->buff_len, BS_CLIENT_BUFF_CAPACITY - parser->buff_len, flags);
   if (bytes_read > 0) {
      if (BS_CLIENT_BUFF_CAPACITY - parser->buff_len < (size_t)bytes_read) {
         DEBUG_FUNC("buffer busy!\n");
         return -1;
      }
      parser->buff_len += bytes_read;
      peer->sock.packets_received++;
      http_parser_consume(&peer->parser, parser->buff, bytes_read);
      
  //    DEBUG_FUNC("bytes read: %zu buff len size: %zu from: %d\n", bytes_read, parser->buff_len, peer->sock.fd);
   } else if (bytes_read == 0) {
      // DEBUG_FUNC("peer disconnected: %d\n", peer->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}
