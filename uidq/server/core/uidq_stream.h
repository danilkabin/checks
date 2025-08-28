#ifndef UIDQ_STREAM_INCLUDE_H
#define UIDQ_STREAM_INCLUDE_H

#include "core/uidq_log.h"
#include "core/uidq_socket.h"
#include <unistd.h>

typedef struct uidq_annihilator_s uidq_annihilator_t ;

struct uidq_annihilator_s {
   uidq_socket_t fd;
   int type;

   struct sockaddr *sockaddr;
   socklen_t socklen;

   int sendbuf;
   int recvbuf;

   unsigned ipv6only:1;

   unsigned reuseaddr:1;
   unsigned reuseport:1;
   
   uidq_log_t *log;
};

typedef struct uidq_zxcghoul_s uidq_zxcghoul_t;

struct uidq_zxcghoul_s {
   uidq_socket_t fd;
   void *data;

   uidq_log_t *log;
};

#endif /* UIDQ_STREAM_INCLUDE_H */
