#include "net.h"
#include "pool.h"
#include "socket.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>

int onion_net_conf_init(onion_net_conf_t *net_conf) {
   const char *ip_phrase = "default";
   size_t ip_phrase_size = strlen(ip_phrase);
   if (strncasecmp(net_conf->ip_address, ip_phrase, ip_phrase_size) == 0) {
      strncpy(net_conf->ip_address, ONION_DEFAULT_IP_ADDRESS, ONION_IP_ADDRESS_SIZE - 1);
      net_conf->ip_address[ONION_IP_ADDRESS_SIZE - 1] = '\0';
   }
   net_conf->port = net_conf->port < 0 ? ONION_DEFAULT_PORT : net_conf->port;
   net_conf->max_peers = net_conf->max_peers < 0 ? ONION_DEFAULT_MAX_PEERS : net_conf->max_peers;
   net_conf->max_queue = net_conf->max_queue < 0 ? ONION_DEFAULT_MAX_QUEUE : net_conf->max_queue;
   return 0;
}

onion_net_static_t *onion_net_priv(onion_server_net *net) {
   return net->parent;
}

onion_peer_net *onion_peer_net_init(onion_server_net *net_server,  onion_net_sock *sock) {
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
   net_server->peer_current = net_server->peer_current + 1;
   return peer;
free_please:
   onion_peer_net_exit(net_server, peer);
unsuccessfull:
   return NULL;
}

void onion_peer_net_exit(onion_server_net *net_server, onion_peer_net *peer) {
   onion_block_free(net_server->peer_barracks, peer); 
}

onion_server_net *onion_server_net_init(onion_net_static_t *net_static) {
   int ret;
   onion_net_conf_t *net_conf = net_static->conf;

   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(net_conf->port),
   };

   if (inet_pton(AF_INET, net_conf->ip_address, &port_conf.addr) <= 0) {
      DEBUG_ERR("Ip address bind failed.\n");
      goto unsuccessfull;
   }

   if (onion_tcp_port_conf_check(&port_conf) < 0) {
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

   size_t size = sizeof(onion_peer_net);
   size_t capable = size * net_conf->max_peers;

   net_server->peer_barracks = onion_block_init(capable, size);
   if (!net_server->peer_barracks) {
      DEBUG_ERR("Net peers initialization failed.\n");
      goto please_free;
   }

   net_server->sock = onion_net_sock_init(&port_conf, net_conf->max_queue);
   if (!net_server->sock) {
      DEBUG_ERR("Server socket initialization failed.\n");
      goto please_free;
   }

   net_server->peer_current = 0;
   net_server->peer_capable = net_conf->max_peers;
   net_server->initialized  = true;
   net_server->parent       = net_static;
   net_static->count        = net_static->count + 1;

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

onion_net_static_t *onion_net_static_init(onion_net_conf_t *net_conf, long capable) {
   int ret;

   onion_net_static_t *net_static = malloc(sizeof(*net_static));
   if (!net_static) {
      DEBUG_ERR("Failed to allocate net_static.\n");
      return NULL;
   }

   net_static->conf = net_conf;
   net_static->count = 0;
   net_static->capable = capable;

   size_t nets_size = sizeof(onion_server_net) * net_static->capable;

   net_static->nets = onion_block_init(net_static->capable * sizeof(onion_server_net), sizeof(onion_server_net)); 
   if (!net_static->nets) {
      DEBUG_ERR("Failed to initialize nets slab (size: %zu).\n", nets_size);
      goto unsuccessfull;
   }

   DEBUG_FUNC("onion_net_static_t initialized (%ld cores).\n", net_static->capable);
   return net_static;
unsuccessfull:
   onion_net_static_exit(net_static);
   return NULL;
}

void onion_net_static_exit(onion_net_static_t *net_static) {
   if (!net_static) return;

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

