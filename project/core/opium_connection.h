#ifndef OPIUM_CONNECTION_INCLUDE_H
#define OPIUM_CONNECTION_INCLUDE_H

#include "core/opium_core.h"

struct opium_listening_s {
   opium_socket_fd_t fd;

    opium_s32_t      port;

   struct sockaddr  *sockaddr;
   socklen_t         socklen;

   size_t            sendbuf;
   size_t            recvbuf;
};

struct opium_connection_s {
   opium_socket_fd_t fd;

   struct sockaddr  *sockaddr;
   socklen_t         socklen;

   opium_u32_t       session_id;

   unsigned          authorized:1;

   size_t            sendbuf;
   size_t            recvbuf;
};

#endif /* OPIUM_CONNECTION_INCLUDE_H */
