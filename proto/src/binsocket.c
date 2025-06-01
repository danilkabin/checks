#include "listhead.h"
#include "main.h"
#include "binsocket.h"
#include "pool.h"
#include "utils.h"
#include "vector.h"

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct memoryPool *bs_client_pool = NULL;
struct list_head bs_client_list = LIST_HEAD_INIT(bs_client_list);

uint8_t *clientIDs = NULL;
size_t clientIDs_size = (MAX_CLIENTS_CAPABLE + 7) / 8;

index_table bs_client_table;
sock_syst_status SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;

sock_syst_status get_socket_system_status() {
   return SOCKET_SYSTEM_STATUS;
}

uint8_t *bs_get_clients_bitmap() {
   return clientIDs;
}

struct binSocket_client *get_bs_client_by_index(int index) {
   return indextable_get(&bs_client_table, index);
}

struct binSocket *binSocket_create(struct tcp_port_conf *port_conf) {
   CHECK_SOCKET_SYSTEM_STATUS(NULL);
   if (!port_conf->port || !port_conf->domain || !port_conf->type) {
      return NULL;
   }

   struct binSocket *sock = malloc(sizeof(struct binSocket));
   if (!sock) {
      return NULL;
   }

   memcpy(&sock->port_conf, port_conf, sizeof(struct tcp_port_conf));
   sock->state = SOCKET_CLOSE;
   DEBUG_FUNC("socket created!\n");
   return sock;
}

int binSocket_init(struct binSocket *sock) {
   CHECK_SOCKET_SYSTEM_STATUS(-1);
   int ret;
   struct tcp_port_conf port_conf = sock->port_conf;

   sock->fd = socket(port_conf.domain, port_conf.type, port_conf.protocol);
   if (sock->fd < 0) {
      return -1;
   }

   sock->sock_addr.sin_addr.s_addr = port_conf.addr.s_addr;
   sock->sock_addr.sin_port = port_conf.port;
   sock->sock_addr.sin_family = port_conf.domain; 
   sock->addrlen = sizeof(sock->sock_addr);

   sock->packets_sent = 0;
   sock->packets_received = 0;

   sock->peer_connections = 0;
   sock->peer_capable = DEFAULT_PEER_CAPABLE;
   sock->peer_queue_capable = DEFAULT_PEER_QUEUE_CAPABLE;

   ret = bind(sock->fd, (struct sockaddr *)&sock->sock_addr, sizeof(sock->sock_addr));
   if (ret < 0) {
      binSocket_release(sock);
      return -1;
   }

   ret = listen(sock->fd, sock->peer_queue_capable);
   if (ret < 0) {
      binSocket_release(sock);
      return -1;
   }

   sock->state = SOCKET_OPEN;
   DEBUG_FUNC("socked inited!\n");
   return 0;
}

void binSocket_release(struct binSocket *sock) {
   if (sock->fd > 0) {
      close(sock->fd);
   }
   free(sock);
}

struct binSocket_client *binSocket_client_create(sockType fd, idType user_id) {
   CHECK_SOCKET_SYSTEM_STATUS(NULL);

   struct binSocket_client *client = (struct binSocket_client*)memoryPool_allocBlock(bs_client_pool);
   if (!client) {
      DEBUG_FUNC("new client initialization was failed!\n");
      goto free_this_trash; 
   }
   client->sock = fd;
   client->user_id = user_id;
   indextable_set(&bs_client_table, client, client->user_id);
   list_add(&client->list, &bs_client_list);
   return client;
free_this_trash:
   return NULL;
}

void binSocket_client_release(struct binSocket_client *client) {
   uint8_t *bitmap = clientIDs;
   if (client->sock) {
      close(client->sock);
   }
   clear_bit(bitmap, client->user_id);
   indextable_set(&bs_client_table, NULL, client->user_id);
   list_del(&client->list);
   memoryPool_freeBlock(bs_client_pool, client);
}

int binSocket_accept(struct binSocket *sock) {
   int ret = -1;
   if (!sock) {
      DEBUG_FUNC("no sock!\n");
      goto free_this_trash;
   }
   CHECK_SOCKET_SYSTEM_STATUS(-1);
   uint8_t *bitmap = clientIDs;
   size_t bitmap_size = MAX_CLIENTS_CAPABLE;

   printBitmap(clientIDs, clientIDs_size);

   int user_id = ffb(bitmap, bitmap_size);
   if (user_id < 0) {
      DEBUG_FUNC("no user id!\n");
      goto free_this_trash;
   }
   set_bit(bitmap, user_id);
   struct sockaddr_in sock_addr;
   socklen_t addrlen = sizeof(sock_addr); 
   sockType client_fd = accept(sock->fd, (struct sockaddr*)&sock_addr, &addrlen);
   if (client_fd < 0) {
      DEBUG_FUNC("accept client descriptor was failed!\n");
      goto clear_bit_yes;
   } 

   struct binSocket_client *client = binSocket_client_create(client_fd, user_id); 
   if (!client) {
      DEBUG_FUNC("new client initialization was failed!\n");
      goto free_sock; 
   }

   return user_id;
free_sock:
   close(client_fd); 
clear_bit_yes:
   clear_bit(bitmap, user_id);
free_this_trash:
   return ret;
}

int sock_syst_init(void) {
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
   int ret = memoryPool_init(&bs_client_pool, MAX_CLIENTS_CAPABLE * sizeof(struct binSocket_client), sizeof(struct binSocket_client));
   if (ret < 0) {
      DEBUG_FUNC("client pool initialization was failed!\n");
      goto free_this_trash;
   }

   clientIDs = malloc(clientIDs_size);
   if (!clientIDs) {
      DEBUG_FUNC("client ids initialization was failed!\n");
      goto free_client_pool;
   }

   indextable_init(&bs_client_table, MAX_CLIENTS_CAPABLE);
   if (!bs_client_table.items) {
      DEBUG_FUNC("no bs_client_table items\n");
      goto free_clientIDs;
   }

   memset(clientIDs, 0, clientIDs_size);
   INIT_LIST_HEAD(&bs_client_list);
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_OPEN;
   return 0;
free_clientIDs:
   free(clientIDs);
free_client_pool:
   memoryPool_free(bs_client_pool);
free_this_trash:
   return ret;
}

void sock_syst_exit(void) {
   struct binSocket_client *child, *tmp;
   list_for_each_entry_safe(child, tmp, &bs_client_list, list) {
      binSocket_client_release(child);
   }
   indextable_exit(&bs_client_table);
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
}
