#ifndef ONION_SEND_H
#define ONION_SEND_H

#include "poll.h"
#include <stddef.h>

size_t onion_peer_sock_send(struct onion_server_sock *, struct onion_peer_sock *, int);

#endif 
