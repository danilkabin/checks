#include "net.h"
#include "pool.h"
#include "socket.h"
#include "utils.h"

onion_net_static_t *big_smoke;

onion_net_static_t *onion_get_static_by_epoll(onion_server_net *net) {
   return big_smoke;
}

int onion_accept_net(onion_server_net *net_server) {
   int ret;
   
   struct onion_net_sock temp_sock;
   ret = onion_net_sock_accept(net_server->sock, &temp_sock);
   if (ret < 0) {
      DEBUG_ERR("Peer finding failed.\n");
      goto unsuccessfull;
   }

   onion_peer_net *peer = onion_peer_net_init(net_server, &temp_sock);
   if (!peer) {
     DEBUG_ERR("Peer init failed!\n");
     goto free_sock;
   }

   DEBUG_FUNC("Peer inited!: %d", peer->initialized);

   return 0;
free_sock:
   onion_net_sock_exit(&temp_sock);
unsuccessfull:
   return -1;
}

onion_peer_net *onion_peer_net_init(onion_server_net *net_server, struct onion_net_sock *sock) {
   int ret;

   if (!net_server->initialized) {
      DEBUG_ERR("Onion server net is not initialized.\n");
      goto unsuccessfull;
   }

   if (net_server->peer_current + 1 >= net_server->peer_capable) {
      DEBUG_ERR("Onion peer net full baby.\n");
      goto unsuccessfull; 
   }

   onion_peer_net *peer = onion_block_alloc(net_server->peer_barracks, NULL);
   if (!peer) {
      DEBUG_FUNC("Onion peer initialization failed.\n");
      goto unsuccessfull;
   }

   peer->sock = sock;
   peer->initialized = true;
   return peer;
free_please:
   onion_peer_net_exit(net_server, peer);
unsuccessfull:
   return NULL;
}

void onion_peer_net_exit(onion_server_net *net_server, onion_peer_net *peer) {
   free(peer);
}

onion_server_net *onion_server_net_init(onion_net_static_t *net_static, onion_server_net_conf *conf) {
   int ret;
   if (conf->peers_capable < 1) {
      DEBUG_ERR("Peers capable is less than 1.\n");
      goto unsuccessfull;
   }

   if (conf->queue_capable < 1) {
      DEBUG_ERR("Queue capable is less than 1.\n");
      goto unsuccessfull;
   }

   if (onion_tcp_port_conf_check(&conf->port_conf) < 0) {
      DEBUG_ERR("Port configuration is invalid.\n");
      goto unsuccessfull;
   }

   if (net_static->count + 1 > net_static->capable) {
      DEBUG_ERR("Net static is full baby.\n");
      goto unsuccessfull;
   }

   onion_server_net *net_server = onion_block_alloc(net_static->nets, NULL);
   if (!net_server) {
      DEBUG_ERR("Server socket struct initialization failed.\n");
      goto unsuccessfull;
   }

   net_server->initialized = false;

   ret = onion_block_init(&net_server->peer_barracks, conf->peers_capable * sizeof(onion_peer_net), sizeof(onion_peer_net));
   if (ret < 0) {
      DEBUG_ERR("Net peers initialization failed.\n");
      goto please_free;
   }

   ret = onion_net_sock_init(&net_server->sock, &conf->port_conf, conf->queue_capable);
   if (ret < 0) {
      DEBUG_ERR("Server socket initialization failed.\n");
      goto please_free;
   }

   net_server->peer_current = 0;
   net_server->peer_capable = conf->peers_capable;
   net_server->initialized = true;
   net_static->count = net_static->count + 1;

   //DEBUG_FUNC("Net server: fd=%d, core=%d, struct size=%zu.\n");
   return net_server;
please_free:
   onion_server_net_exit(net_static, net_server);
unsuccessfull:
   return NULL;
}

void onion_server_net_exit(onion_net_static_t *net_static, onion_server_net *net_server) {
   net_server->initialized = false;

   if (net_server->sock) {
      onion_net_sock_exit(net_server->sock);
      net_server->sock = NULL;
   }

   if (net_server->peer_barracks) {
      onion_block_exit(net_server->peer_barracks);
      net_server->peer_barracks = NULL;
   }

   free(net_server);
   net_static->count = net_static->count - 1;
}

int onion_net_static_init(onion_net_static_t **ptr, long capable) {
   int ret;
 
   if (big_smoke) {
      DEBUG_ERR("Big smoke already existing!\n");
      return -1;
   }
 
   onion_net_static_t *net_static = malloc(sizeof(*net_static));
   if (!net_static) {
      DEBUG_ERR("Failed to allocate net_static.\n");
      return -1;
   }

   net_static->count = 0;
   net_static->capable = capable;

   size_t nets_size = sizeof(onion_server_net) * net_static->capable;

   ret = onion_block_init(&net_static->nets, net_static->capable * sizeof(onion_server_net), sizeof(onion_server_net)); 
   if (ret < 0) {
      DEBUG_ERR("Failed to initialize nets slab (size: %zu).\n", nets_size);
      goto unsuccessfull;
   }

   big_smoke = net_static;
   *ptr = net_static;
   DEBUG_FUNC("onion_net_static_t initialized (%ld cores).\n", net_static->capable);
   return 0;
unsuccessfull:
   onion_net_static_exit(net_static);
   return -1;
}

void onion_net_static_exit(onion_net_static_t *net_static) {
   if (!net_static) return;

   big_smoke = NULL;

   if (net_static->nets) {
      for (size_t index = 0; index < (size_t)net_static->nets->block_max; ++index) {
         onion_server_net *net = onion_block_get(net_static->nets, index);
         if (net) {
            onion_server_net_exit(net_static, net); 
         }
      }

      onion_block_exit(net_static->nets);
      net_static->nets = NULL; 
   }

   net_static->count = 0;
   net_static->capable = 0;

   free(net_static);
   DEBUG_FUNC("onion net static exit.\n");
}

