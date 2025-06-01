#ifndef FLOWBIN_PEER_H
#define FLOWBIN_PEER_H

#include "binsocket.h"
#include "listhead.h"
#include "main.h"
#include "pool.h"

extern struct list_head fb_peer_list;
extern struct memoryPool *fb_peer_pool;

struct fb_peer_priv {
    struct binSocket *sock;
    struct list_head list;
    char buffer[DEFAULT_PEER_BUFFER_CAPACITY];
    uint32_t buffer_capacity;
    uint32_t current_packet_size;
    uint32_t current_received_size;
    BINSOCKET_HEADER current_header;
    MESSAGE_STATE message_state;
};

int fb_new_peer(struct fb_peer_priv **peer, struct binSocket *sock);
void fb_clear_peer(struct fb_peer_priv *peer);

int fb_peers_init(void);
void fb_peers_release(void);

#define DEFAULT_PEER_PRIV_SIZE (sizeof(struct fb_peer_priv))
#define DEFAULT_PEER_POOL_SIZE (MAXIMUM_ONLINE_CLIENTS * DEFAULT_PEER_PRIV_SIZE)

#endif
