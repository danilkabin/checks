#include "listhead.h"
#include "lock.h"
#include "main.h"
#include "binsocket.h"
#include "pool.h"
#include "utils.h"
#include "vector.h"

#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <urcu/urcu-memb.h>
#include <fcntl.h>

struct memoryPool *bs_client_pool = NULL;

index_table bs_client_table;
sock_syst_status SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;

sock_syst_status get_socket_system_status() {
   return SOCKET_SYSTEM_STATUS;
}

uint8_t *bs_get_clients_bitmap(struct server_sock *bs) {
   return bs->clientIDs;
}

struct client_sock *get_bs_client_by_index(int index) {
   return indextable_get(&bs_client_table, index);
}

int sock_t_init(sock_t *sock) {
   sock->state = SOCKET_OPEN;
   sock->packets_sent = 0;
   sock->packets_received = 0;
   return 0;
}

struct server_sock *server_sock_create(struct tcp_port_conf *port_conf) {
   CHECK_SOCKET_SYSTEM_STATUS(NULL);
   int ret;
   if (!port_conf->port || !port_conf->domain || !port_conf->type) {
      goto free_this_trash;
   }

   struct server_sock *bs = malloc(sizeof(struct server_sock));
   if (!bs) {
      DEBUG_ERR("no binsocket!\n");
      goto free_this_trash;
   }

   atomic_store(&bs->released, false);

   bs->sock.fd = socket(port_conf->domain, port_conf->type, port_conf->protocol);
   if (bs->sock.fd < 0) {
      DEBUG_FUNC("server sock descriptor fd wasn`t inited: %s\n", strerror(errno));
      goto free_everything;
   }

   memcpy(&bs->port_conf, port_conf, sizeof(struct tcp_port_conf));
   bs->sock.state = SOCKET_CLOSE;
   bs->peer_connections = 0;
   bs->peer_capable = DEFAULT_PEER_CAPABLE;
   bs->peer_queue_capable = DEFAULT_PEER_QUEUE_CAPABLE;

   bs->epoll_fd = epoll_create1(0);
   if (bs->epoll_fd < 0) {
      DEBUG_ERR("no epoll_fd\n");
      goto free_everything; 
   }

   bs->event.events = EPOLLIN;
   bs->event.data.fd = bs->sock.fd;

   ret = epoll_ctl(bs->epoll_fd, EPOLL_CTL_ADD, bs->sock.fd, &bs->event);
   if (ret < 0) {
      DEBUG_ERR("no epoll_ctl\n");
      goto free_everything;
   }

   bs->clientIDs_size = (MAX_CLIENTS_CAPABLE + 7) / 8;
   bs->clientIDs = malloc(bs->clientIDs_size);
   if (!bs->clientIDs) {
      DEBUG_ERR("no clientIDs\n");
      goto free_everything;
   }
   memset(bs->clientIDs, 0, bs->clientIDs_size);

   INIT_LIST_HEAD(&bs->clients);
   mutex_init(&bs->client_core.mutex, NULL);
   DEBUG_FUNC("socket created!\n");
   return bs;
free_everything:
   server_sock_release(bs);
free_this_trash:
   return NULL;
}

int server_sock_init(struct server_sock *bs) {
   CHECK_SOCKET_SYSTEM_STATUS(-1);
   int ret;

   struct tcp_port_conf port_conf = bs->port_conf;

   bs->sock_addr.sin_addr.s_addr = port_conf.addr.s_addr;
   bs->sock_addr.sin_port = port_conf.port;
   bs->sock_addr.sin_family = port_conf.domain;
   bs->addrlen = sizeof(bs->sock_addr);

   ret = bind(bs->sock.fd, (struct sockaddr *)&bs->sock_addr, sizeof(bs->sock_addr));
   if (ret < 0) {
      server_sock_release(bs);
      return -1;
   }

   ret = listen(bs->sock.fd, bs->peer_queue_capable);
   if (ret < 0) {
      server_sock_release(bs);
      return -1;
   }

   sock_t_init(&bs->sock);
   DEBUG_FUNC("socket inited!\n");
   return 0;
free_this_trash:
   return -1;
}

void server_sock_release(struct server_sock *bs) {
   if (atomic_exchange(&bs->released, true)) {
      return;
   }
   atomic_store(&bs->released, true);
   struct client_sock *client, *tmp;
   mutex_lock(&bs->client_core.mutex);
   list_for_each_entry_safe(client, tmp, &bs->clients, list) {
      client_sock_release(bs, client);
   }
   mutex_unlock(&bs->client_core.mutex);
   synchronize_rcu();

   if (bs->sock.fd > 0) {
      close(bs->sock.fd);
   }

   if (bs->epoll_fd > 0) {
      close(bs->epoll_fd);
   }

   if (bs->clientIDs) {
      free(bs->clientIDs);
      bs->clientIDs = NULL;
   }

   mutex_destroy(&bs->client_core.mutex);
   free(bs);
}

struct client_sock *client_sock_create(struct server_sock *bs, sockType fd) {
   CHECK_SOCKET_SYSTEM_STATUS(NULL);
   int ret;
   int fcntl_result = fcntl(fd, F_GETFL, 0);
   if (fcntl_result == -1) {
      DEBUG_ERR("ZHENYA DYRA!\n");
      goto free_this_trash;
   }

   fcntl_result = fcntl(fd, F_SETFL, fcntl_result | O_NONBLOCK);
   if (fcntl_result == -1) {
      DEBUG_ERR("no client flag set!\n");
      goto free_this_trash;
   } 

   struct client_sock *client = (struct client_sock*)memoryPool_allocBlock(bs_client_pool);
   if (!client) {
      DEBUG_ERR("new client initialization was failed!\n");
      goto free_this_trash;
   }

   atomic_store(&client->released, false);
   client->communicate_epoll.events = EPOLLIN | EPOLLOUT | EPOLLET;
   client->communicate_epoll.data.fd = fd;
   ret = epoll_ctl(bs->epoll_fd, EPOLL_CTL_ADD, fd, &client->communicate_epoll);
   if (ret < 0) {
      DEBUG_ERR("no epoll_ctl\n");
      goto free_everything;
   }

   client->sock.fd = fd;
   client->buff_len = 0;

   mutex_lock(&bs->client_core.mutex);
   list_add_rcu(&client->list, &bs->clients);
   mutex_unlock(&bs->client_core.mutex);

   return client;
free_everything:
   client_sock_release(bs, client); 
free_this_trash:
   return NULL;
}

void client_sock_release(struct server_sock *bs, struct client_sock *client) {
   if (atomic_exchange(&client->released, true)) {
      return;
   }
   atomic_store(&client->released, true);
   int ret;

   if (client->sock.fd > 0) {
      ret = epoll_ctl(bs->epoll_fd, EPOLL_CTL_DEL, client->sock.fd, &client->communicate_epoll);
      if (ret == -1 && errno != ENOENT) {
         DEBUG_FUNC("event delete failed!\n");
      }
      close(client->sock.fd);
   }

   mutex_lock(&bs->client_core.mutex);
   list_del_rcu(&client->list);
   mutex_unlock(&bs->client_core.mutex);
   synchronize_rcu();

   memoryPool_freeBlock(bs_client_pool, client);
}

int server_sock_accept(struct server_sock *bs, accept_callback_sk work) {
   int ret = -1;

   if (!bs || !work) {
      DEBUG_FUNC("no sock or work!\n");
      goto free_this_trash;
   }

   CHECK_SOCKET_SYSTEM_STATUS(-1);

   struct sockaddr_in sock_addr;
   socklen_t addrlen = sizeof(sock_addr);
   sockType client_fd = accept(bs->sock.fd, (struct sockaddr*)&sock_addr, &addrlen);
   if (client_fd < 0) {
      DEBUG_ERR("accept client descriptor was failed!\n");
      goto free_this_trash;
   }

   struct client_sock *client = client_sock_create(bs, client_fd);
   if (!client) {
      DEBUG_ERR("client initialization was failed\n");
      goto free_sock;
   }

   work(client_fd, client);
   return client_fd;

free_sock:
   close(client_fd);
free_this_trash:
   return ret;
}

size_t client_sock_recv(struct client_sock *client, size_t buff_size, int flags) {
   if (!client) {
      DEBUG_FUNC("no sock\n");
      return -1;
   }
   size_t bytes_read = recv(client->sock.fd, client->buff + client->buff_len, buff_size - client->buff_len, flags);
   if (bytes_read > 0) {
      client->buff_len += bytes_read;
      client->sock.packets_received++;
      DEBUG_FUNC("bytes read: %zu from: %d\n", bytes_read, client->sock.fd);
   } else if (bytes_read == 0) {
      DEBUG_FUNC("client disconnected: %d\n", client->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}

int sock_syst_init(void) {
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;

   int ret = memoryPool_init(&bs_client_pool, MAX_CLIENTS_CAPABLE * sizeof(struct client_sock), sizeof(struct client_sock));
   if (ret < 0) {
      DEBUG_FUNC("client pool initialization was failed!\n");
      goto free_this_trash;
   }

   indextable_init(&bs_client_table, MAX_CLIENTS_CAPABLE);
   if (!bs_client_table.items) {
      DEBUG_FUNC("no bs_client_table items\n");
      goto free_client_pool;
   }

   /* ret = pthread_create(&bs->anonymous_core.flow, NULL, bs_anonymous_runservice, NULL);
      if (ret != 0) {
      DEBUG_FUNC("no thread!\n");
      goto free_indextable;
      }*/

   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_OPEN;
   return 0;

free_indextable:
   indextable_exit(&bs_client_table);
free_client_pool:
   memoryPool_free(bs_client_pool);
free_this_trash:
   return ret;
}

void sock_syst_exit(struct server_sock *bs) {
   struct client_sock *child, *tmp;
   void *ac_result;

   // pthread_join(anonymous_core.flow, &ac_result);

   mutex_synchronise(&bs->client_core.mutex);
   mutex_lock(&bs->client_core.mutex);
   list_for_each_entry_safe(child, tmp, &bs->clients, list) {
      client_sock_release(bs, child);
   }
   mutex_unlock(&bs->client_core.mutex);

   indextable_exit(&bs_client_table);
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
}
