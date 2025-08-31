#ifndef OPIUM_STREAM_INCLUDE_H
#define OPIUM_STREAM_INCLUDE_H

#include "core/opium_log.h"
#include "core/opium_socket.h"
#include <unistd.h>

typedef struct opium_annihilator_s opium_annihilator_t;

struct opium_annihilator_s {
   opium_socket_t fd;
   int type;

   struct sockaddr *sockaddr;
   socklen_t socklen;

   int sendbuf;
   int recvbuf;

   unsigned ipv6only:1;

   unsigned reuseaddr:1;
   unsigned reuseport:1;
   
   opium_log_t *log;
};

typedef struct opium_zxcghoul_s opium_zxcghoul_t;

struct opium_zxcghoul_s {
   opium_socket_t fd;
   void *data;

   opium_log_t *log;
};

#endif /* OPIUM_STREAM_INCLUDE_H */
