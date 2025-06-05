#include "listhead.h"
#include "lock.h"
#include "main.h"
#include "binsocket.h"
#include "pool.h"
#include "utils.h"
#include "vector.h"

#include <arpa/inet.h>
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

void worker_release(struct worker *work) {
   if (work->epoll_fd) {
      close(work->epoll_fd);
      work->epoll_fd = -1;
   }
   if (work->core.flow) {
      pthread_cancel(work->core.flow);
      pthread_join(work->core.flow, NULL);
      work->core.flow = 0;
   }
   work->bs = NULL;
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

   atomic_store(&bs->initialized, false);
   atomic_store(&bs->released, false);

   bs->sock.fd = socket(port_conf->domain, port_conf->type, port_conf->protocol);
   if (bs->sock.fd < 0) {
      DEBUG_FUNC("server sock descriptor fd wasn`t inited: %s\n", strerror(errno));
      goto free_everything;
   }

   ret = memoryPool_init(&bs->client_pool, MAX_CLIENTS_CAPABLE * sizeof(struct client_sock), sizeof(struct client_sock));
   if (ret < 0) {
      DEBUG_FUNC("client pool initialization was failed!\n");
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
   DEBUG_ERR("yeseys\n");
   struct tcp_port_conf port_conf = bs->port_conf;

   bs->sock_addr.sin_addr.s_addr = port_conf.addr.s_addr;
   bs->sock_addr.sin_port = port_conf.port;
   bs->sock_addr.sin_family = port_conf.domain;
   bs->addrlen = sizeof(bs->sock_addr);

   CHECK_MY_THREAD(goto free_everything, &bs->epollable_thread, NULL, epollable_thread, bs);
   CHECK_MY_THREAD(goto free_everything, &bs->accept_thread, NULL, bs_accept_thread, bs);
   CHECK_MY_THREAD(goto free_everything, &bs->client_recv_thread, NULL, client_recv_thread, bs);

   mutex_init(&bs->epollable_thread.mutex, NULL);
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
   atomic_store(&bs->initialized, true);

   return 0;
free_everything:
   DEBUG_FUNC("yes\n");
   server_sock_release(bs);
free_this_trash:
   return -1;
}

void server_sock_release(struct server_sock *bs) {
   if (atomic_load(&bs->released)) {
      return;
   }
   atomic_store(&bs->released, true);

   struct client_sock *client, *tmp;
   void *ac_result;

   mutex_lock(&bs->epollable_thread.mutex);
   list_for_each_entry_safe(client, tmp, &bs->clients, list) {
      client_sock_release(bs, client);
   }
   mutex_unlock(&bs->epollable_thread.mutex);
   synchronize_rcu();

   for (int worker_index = 0; worker_index < WORKER_COUNT; worker_index++) {
      struct worker *work = bs->workers[worker_index];
      worker_release(work);
   }

   if (bs->sock.fd > 0) {
      close(bs->sock.fd);
   }

   if (bs->epoll_fd > 0) {
      close(bs->epoll_fd);
   }

   if (bs->client_pool) {
      memoryPool_free(bs->client_pool);
      bs->client_pool = NULL;
   }

   if (bs->clientIDs) {
      free(bs->clientIDs);
      bs->clientIDs = NULL;
   }

   pthread_join(bs->epollable_thread.flow, &ac_result);

   mutex_destroy(&bs->epollable_thread.mutex);
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

   struct client_sock *client = (struct client_sock*)memoryPool_allocBlock(bs->client_pool);
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

   mutex_lock(&bs->epollable_thread.mutex);
   list_add_rcu(&client->list, &bs->clients);
   mutex_unlock(&bs->epollable_thread.mutex);

   return client;
free_everything:
   client_sock_release(bs, client); 
free_this_trash:
   return NULL;
}

void client_sock_release(struct server_sock *bs, struct client_sock *client) {
   if (atomic_load(&client->released)) {
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

   mutex_lock(&bs->epollable_thread.mutex);
   list_del_rcu(&client->list);
   mutex_unlock(&bs->epollable_thread.mutex);
   synchronize_rcu();

   memoryPool_freeBlock(bs->client_pool, client);
}

int server_sock_accept(struct server_sock *bs, accept_callback_sk work) {
   int ret = -1;

   if (!bs) {
      DEBUG_FUNC("no sock!\n");
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

void *client_recv_thread(void *args) {

   return NULL;
}

void *bs_accept_thread(void *args) {

   return NULL;
}

void *bs_worker_thread(void *args) {

   return NULL;
}

void *epollable_thread(void *args) {
   struct server_sock *bs = (struct server_sock*)args;
   if (!bs) {
      DEBUG_ERR("no server_sock\n");
      goto free_this_trash;
   }

   for (int work_index = 0; work_index < WORKER_COUNT; work_index++) {
      struct worker *work = bs->workers[work_index];
      work->epoll_fd = epoll_create1(0);
      if (work->epoll_fd < 0) {
         DEBUG_ERR("flows weren`t inited\n");
         return NULL;
      } 
      work->bs = bs;
      CHECK_MY_THREAD(goto free_workers, &work->core, NULL, bs_worker_thread, work);
   }

   while (!atomic_load(&bs->released)) {
      if (!atomic_load(&bs->initialized)) {
         continue;
      }

   }

   DEBUG_INFO("yes yes");
   return NULL;
free_workers:
   for (int worker_index = 0; worker_index < WORKER_COUNT; worker_index++) {
      struct worker *work = bs->workers[worker_index];
      worker_release(work);
   }
free_this_trash:
   return NULL;
}

int sock_syst_init(void) {
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
   int ret = -1;

   indextable_init(&bs_client_table, MAX_CLIENTS_CAPABLE);
   if (!bs_client_table.items) {
      DEBUG_FUNC("no bs_client_table items\n");
      goto free_indextable;
   }

   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_OPEN;
   return 0;

free_indextable:
   indextable_exit(&bs_client_table);
free_this_trash:
   return ret;
}

void sock_syst_exit(struct server_sock *bs) {
   struct client_sock *child, *tmp;

   mutex_synchronise(&bs->epollable_thread.mutex);
   mutex_lock(&bs->epollable_thread.mutex);
   list_for_each_entry_safe(child, tmp, &bs->clients, list) {
      client_sock_release(bs, child);
   }
   mutex_unlock(&bs->epollable_thread.mutex);

   indextable_exit(&bs_client_table);
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
}
