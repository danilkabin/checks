#include "net.h"
#include "pool.h"
#include "socket.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int onion_net_conf_init(onion_net_conf_t *net_conf) {
   const char *ip_phrase = "default";
   size_t ip_phrase_size = strlen(ip_phrase);

   if (strncasecmp(net_conf->ip_address, ip_phrase, ip_phrase_size) == 0) {
      strncpy(net_conf->ip_address, ONION_DEFAULT_IP_ADDRESS, ONION_IP_ADDRESS_SIZE - 1);
      net_conf->ip_address[ONION_IP_ADDRESS_SIZE - 1] = '\0';
   }

   if (net_conf->port < 0)
      net_conf->port = ONION_DEFAULT_PORT;
   if (net_conf->max_peers < 0)
      net_conf->max_peers = ONION_DEFAULT_MAX_PEERS;
   if (net_conf->max_queue < 0)
      net_conf->max_queue = ONION_DEFAULT_MAX_QUEUE;

   return 0;
}

onion_net_static_t *onion_net_priv(onion_server_net_t *net) {
   return net->parent;
}

onion_server_slot_net_t *onion_peer_net_init(onion_server_net_t *net_server, onion_net_sock *sock) {
   if (!net_server || !net_server->initialized) {
      DEBUG_ERR("Onion server net is not initialized.\n");
      return NULL;
   }

   if (net_server->peer_current >= net_server->peer_capable) {
      DEBUG_ERR("Onion peer net full.\n");
      return NULL;
   }

   onion_server_slot_net_t *slot = onion_block_alloc(net_server->peer_barracks, NULL);
   if (!slot) {
      DEBUG_ERR("Onion peer initialization failed.\n");
      return NULL;
   }
   slot->initialized = true;
   net_server->peer_current = net_server->peer_current + 1; 

   slot->sock = sock;

   return slot;
}

void onion_peer_net_exit(onion_server_net_t *net_server, onion_server_slot_net_t *slot) {
   if (!net_server || !net_server->initialized) {
      DEBUG_ERR("net_server is NULL or not initialized.\n");  
      return;
   }
   if (!slot || !slot->initialized) {
      DEBUG_ERR("slot is NULL or not initialized.\n");  
      return;
   }
   onion_block_free(net_server->peer_barracks, slot);
   net_server->peer_current--;
}

onion_server_net_t *onion_server_net_init(onion_net_static_t *net_static) {
   onion_net_conf_t *net_conf = net_static->conf;

   struct onion_tcp_port_conf port_conf = {
      .domain = AF_INET,
      .type = SOCK_STREAM,
      .port = htons(net_conf->port),
   };

   if (inet_pton(AF_INET, net_conf->ip_address, &port_conf.addr) <= 0) {
      DEBUG_ERR("IP address bind failed.\n");
      return NULL;
   }

   if (onion_tcp_port_conf_check(&port_conf) < 0) {
      DEBUG_ERR("Port configuration is invalid.\n");
      return NULL;
   }

   if (net_static->count >= net_static->capable) {
      DEBUG_ERR("Net static is full.\n");
      return NULL;
   }

   onion_server_net_t *net_server = onion_block_alloc(net_static->nets, NULL);
   if (!net_server) {
      DEBUG_ERR("Server socket struct initialization failed.\n");
      return NULL;
   }

   net_server->initialized = true;
   net_server->peer_current = 0;
   net_server->peer_capable = net_conf->max_peers;
   net_server->parent = net_static;
   net_static->count = net_static->count + 1;

   size_t peer_struct_size = sizeof(onion_server_slot_net_t);
   size_t peer_total_size = net_conf->max_peers * peer_struct_size;

   net_server->peer_barracks = onion_block_init(peer_total_size, peer_struct_size);
   if (!net_server->peer_barracks) {
      DEBUG_ERR("Net peers initialization failed.\n");
      onion_block_free(net_static->nets, net_server);
      return NULL;
   }

   net_server->sock = onion_net_sock_init(&port_conf, net_conf->max_queue);
   if (!net_server->sock) {
      DEBUG_ERR("Server socket initialization failed.\n");
      onion_block_exit(net_server->peer_barracks);
      onion_block_free(net_static->nets, net_server);
      return NULL;
   }

   return net_server;
}

void onion_server_net_exit(onion_net_static_t *net_static, onion_server_net_t *net_server) {
   if (!net_static || !net_static->initialized) {
      DEBUG_ERR("net_static is NULL or not initialized.\n");  
      return;
   }

   if (!net_server || !net_server->initialized) { 
      DEBUG_ERR("net_server is NULL or not initialized.\n"); 
      return;
   }
   net_server->initialized = false;
   net_static->count = net_static->count - 1;   

   if (net_server->sock) {
      onion_net_sock_exit(net_server->sock);
      net_server->sock = NULL;
   }

   if (net_server->peer_barracks) {
      onion_block_exit(net_server->peer_barracks);
      net_server->peer_barracks = NULL;
   }

   onion_block_free(net_static->nets, net_server);
}

onion_net_static_t *onion_net_static_init(onion_net_conf_t *net_conf, long capable) {
   onion_net_static_t *net_static = malloc(sizeof(*net_static));
   if (!net_static) {
      DEBUG_ERR("Failed to allocate net_static.\n");
      return NULL;
   }
   net_static->initialized = true;

   net_static->conf = net_conf;
   net_static->count = 0;
   net_static->capable = capable;

   net_static->nets = onion_block_init(capable * sizeof(onion_server_net_t), sizeof(onion_server_net_t));
   if (!net_static->nets) {
      DEBUG_ERR("Failed to initialize nets slab.\n");
      free(net_static);
      return NULL;
   }

   DEBUG_FUNC("onion_net_static_t initialized (%ld cores).\n", net_static->capable);
   return net_static;
}

void onion_net_static_exit(onion_net_static_t *net_static) {
   if (!net_static || !net_static->initialized) {
      DEBUG_ERR("net_static is NULL or not initialized.\n");  
      return;
   }

   if (net_static->nets) {
      onion_block_exit(net_static->nets);
      net_static->nets = NULL;
   }

   net_static->count = 0;
   net_static->capable = 0;

   free(net_static);
   DEBUG_FUNC("onion net static exit.\n");
}
