#ifndef BS_RECEIVE_H
#define BS_RECEIVE_H

#include "binsocket.h"
#include <stddef.h>

size_t peer_sock_recv(struct server_sock *, struct peer_sock *, int, size_t);

#endif
