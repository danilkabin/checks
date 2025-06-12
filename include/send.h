#ifndef BS_SEND43_H
#define BS_SEND43_H

#include "binsocket.h"
#include <stddef.h>

size_t peer_sock_send(struct server_sock *, struct peer_sock *, int);

#endif 
