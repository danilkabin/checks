#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "listhead.h"
#include "utils.h"
#include "main.h"
#include "pool.h"
#include "peer.h"
#include "binsocket.h"

struct list_head fb_peer_list = LIST_HEAD_INIT(fb_peer_list);
struct memoryPool *fb_peer_pool = NULL;

static const size_t peer_pool_size = DEFAULT_PEER_POOL_SIZE;

int fb_new_peer(struct fb_peer_priv **peerPtr, struct binSocket *sock) {
   int ret = -1;
   *peerPtr = (struct fb_peer_priv*)memoryPool_allocBlock(fb_peer_pool);
   if (!*peerPtr) {
      DEBUG_FUNC("new peer block wasn`t initialized\n");
      goto free_this_trash;
   }
   struct fb_peer_priv *peer = *peerPtr;
   size_t buffer_capacity = DEFAULT_PEER_BUFFER_CAPACITY;

   peer->sock = sock;
   peer->current_received_size = 0;
   peer->current_packet_size = 0;
   peer->buffer_capacity = buffer_capacity;
   peer->message_state = WAITING_FOR_HANDLER;
   memset(peer->buffer, 0, buffer_capacity);
   
   list_add(&peer->list, &fb_peer_list);
   return ret;
free_this_trash:
   return ret;
}

void fb_clear_peer(struct fb_peer_priv *peer) {
    list_del(&peer->list);
    memoryPool_freeBlock(fb_peer_pool, peer);
}

int fb_peers_init(void) {
    int ret = memoryPool_init(&fb_peer_pool, peer_pool_size, DEFAULT_PEER_PRIV_SIZE);
    if (ret < 0) {
        DEBUG_FUNC("peer pool wasn't initialized\n");
        return -1;
    }
    return 0;
}

void fb_peers_release(void) {

}
