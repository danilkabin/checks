#include "parser.h"
#include "request.h"
#include "utils.h"
#include "receive.h"
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

size_t onion_peer_sock_recv(struct onion_server_sock *bs, struct onion_peer_sock *peer, int flags, size_t buff_size) {
   if (!bs || !peer) {
      DEBUG_FUNC("no bs/peer\n");
      return (size_t)-1;
   }
  onion_http_parser_t *parser = &peer->parser;

   char buff[8192];
   size_t buff_size1 = sizeof(buff);
   memset(buff, 0, buff_size1);

   ssize_t bytes_read = recv(peer->sock.fd, buff, buff_size1, flags);
   if (bytes_read > 0) {
      int ret =onion_http_parser_request(parser, buff, bytes_read);
      peer->sock.packets_received++;
  //    DEBUG_FUNC("bytes read: %zu buff len size: %zu from: %d\n", bytes_read, parser->buff_len, peer->sock.fd);
   } else if (bytes_read == 0) {
      // DEBUG_FUNC("peer disconnected: %d\n", peer->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}
