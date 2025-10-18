#ifndef OPIUM_CONNECTION_INCLUDE_H
#define OPIUM_CONNECTION_INCLUDE_H

#include "core/opium_core.h"

struct opium_listening_s {
   opium_socket_fd_t fd; 

   struct sockaddr   *sockaddr;
   socklen_t          socklen;

   opium_uint_t       worker;

   int                sendbuf;
   int                rcvbuf;

   unsigned           open:1;
   unsigned           reuseport:1;
   unsigned           ipv6only:1;

   opium_arena_t      arena;

   opium_log_t       *log;
};

struct opium_connection_s {
   opium_uint_t       id;

   opium_listening_t *listening;

   struct sockaddr   *sockaddr;
   socklen_t          socklen;


   size_t             bytes_sent;
   size_t             bytes_received;

   unsigned           active:1;
   unsigned           closed:1;

   opium_log_t       *log;
};

#endif /* OPIUM_CONNECTION_INCLUDE_H  */
