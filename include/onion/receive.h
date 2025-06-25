#ifndef ONION_RECEIVE_H
#define ONION_RECEIVE_H

#include "poll.h"
#include "onion.h"
#include <stddef.h>

size_t onion_peer_sock_recv(struct onion_server_sock *, struct onion_peer_sock *, int, size_t);

#endif
