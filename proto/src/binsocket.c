#include <asm-generic/socket.h>
#define _GNU_SOURCE

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
#include <sched.h>
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

int CORE_COUNT;
struct worker **sock_workers;
sock_syst_status SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;

sock_syst_status get_socket_system_status() {
   return SOCKET_SYSTEM_STATUS;
}

uint8_t *bs_get_clients_bitmap(struct server_sock *bs) {
   return bs->clientIDs;
}

void set_worker_affinity(pthread_t thread, int core_id) {
   DEBUG_FUNC("core count: %d\n", core_id);
   cpu_set_t set_t;
   CPU_ZERO(&set_t);
   CPU_SET(core_id, &set_t);
   int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &set_t);
   if (ret != 0) {
      DEBUG_ERR("NO CORE!\n");
   }
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
   free(work);
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

   _CREATE_EPOLL(goto free_everything, bs->epoll_core_fd, bs->core_event, 0);

   bs->core_event.events = EPOLLIN;
   bs->core_event.data.fd = bs->sock.fd;
   bs->core_event.data.ptr = bs;

   ret = epoll_ctl(bs->epoll_core_fd, EPOLL_CTL_ADD, bs->sock.fd, &bs->core_event);
   if(ret < 0) {
      DEBUG_ERR("no epoll_ctl\n");
      goto free_everything;
   }

   bs->workers = malloc(sizeof(struct worker*) * CORE_COUNT);
   if (!bs->workers) {
      DEBUG_ERR("no pool workers!\n");
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
   struct tcp_port_conf port_conf = bs->port_conf;

   bs->sock_addr.sin_addr.s_addr = port_conf.addr.s_addr;
   bs->sock_addr.sin_port = port_conf.port;
   bs->sock_addr.sin_family = port_conf.domain;
   bs->addrlen = sizeof(bs->sock_addr);

   CHECK_MY_THREAD(goto free_everything, &bs->epollable_thread, NULL, epollable_thread, bs);

   mutex_init(&bs->epollable_thread.mutex, NULL);

   int turn = 1;
   ret = setsockopt(bs->sock.fd, SOL_SOCKET, SO_REUSEPORT, &turn, sizeof(turn));
   if (ret < 0) {
      DEBUG_ERR("no set sock options\n");
      goto free_everything;
   }

   ret = bind(bs->sock.fd, (struct sockaddr *)&bs->sock_addr, sizeof(bs->sock_addr));
   if (ret < 0) {
      DEBUG_ERR("no set bind options\n");
      goto free_everything;
   }

   ret = listen(bs->sock.fd, bs->peer_queue_capable);
   if (ret < 0) {
      DEBUG_ERR("no set listen options\n");
      goto free_everything;
   }

   sock_t_init(&bs->sock);
   atomic_store(&bs->initialized, true);

   return 0;
free_everything:
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

   for (int worker_index = 0; worker_index < CORE_COUNT; worker_index++) {
      struct worker *work = bs->workers[worker_index];
      worker_release(work);
   }

   if (bs->sock.fd > 0) {
      close(bs->sock.fd);
   }

   if (bs->client_pool) {
      memoryPool_free(bs->client_pool);
      bs->client_pool = NULL;
   }

   if (bs->clientIDs) {
      free(bs->clientIDs);
      bs->clientIDs = NULL;
   }

   if (bs->workers) {
      free(bs->workers);
   }

   _DELETE_EPOLL(bs->epoll_core_fd);
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
      struct worker *current_worker = client->current_worker;
      if (current_worker) {
         epoll_ctl(current_worker->epoll_fd, EPOLL_CTL_DEL, client->sock.fd, NULL);
         atomic_fetch_sub(&current_worker->client_count, 1);
      }
      close(client->sock.fd);
   }

   mutex_lock(&bs->epollable_thread.mutex);
   list_del_rcu(&client->list);
   mutex_unlock(&bs->epollable_thread.mutex);
   synchronize_rcu();

   memoryPool_freeBlock(bs->client_pool, client);
   DEBUG_FUNC("client sock was released!\n");
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

   struct worker *worker = NULL;
   int min_works = MAX_CLIENTS_CAPABLE + 1;
   int threadd = 0;

   for (int worker_index = 0; worker_index < CORE_COUNT; worker_index++) {
      struct worker *work = bs->workers[worker_index];
      int client_count = atomic_load_explicit(&work->client_count, memory_order_relaxed);
      if (client_count < min_works) {
         min_works = client_count;
         worker = work;
         threadd = worker_index;
      }
   }
   DEBUG_FUNC("thread:%d\n",threadd);
   if (!worker) {
      DEBUG_ERR("no worker!\n");
      goto free_everything;
   }

   struct epoll_event client_event;
   client_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
   client_event.data.ptr = client;

   ret = epoll_ctl(worker->epoll_fd, EPOLL_CTL_ADD, client->sock.fd, &client_event);

   if (ret == -1) {
      DEBUG_ERR("wasn`t added client to epoll fd");
      goto free_everything;
   }
   client->current_worker = worker;
   atomic_fetch_add(&worker->client_count, 1);
   return client_fd;
free_everything:
   client_sock_release(bs, client);
   return ret;
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
   size_t bytes_read = recv(client->sock.fd, client->buff, buff_size, flags);
   client->buff_len = 0;
   if (bytes_read > 0) {
      client->buff_len += bytes_read;
      client->sock.packets_received++;
      DEBUG_FUNC("bytes read: %zu from: %d core: %d\n", bytes_read, client->sock.fd, client->current_worker->core_id);
   } else if (bytes_read == 0) {
      DEBUG_FUNC("client disconnected: %d\n", client->sock.fd);
   } else {
      DEBUG_FUNC("recv failed: %s\n", strerror(errno));
   }
   return bytes_read;
}

void *bs_worker_thread(void *args) {
   struct worker *work = (struct worker*)args;
   if (!work) {
      DEBUG_ERR("no worker!\n");
      return NULL;
   }
   struct server_sock *bs = work->bs;
   if (!bs) {
      DEBUG_ERR("no server sock\n");
      return NULL;
   }
   set_worker_affinity(work->core.flow, work->core_id);
   int timeout = -1;
   while (!atomic_load(&bs->released)) {
      if (!atomic_load(&bs->initialized)) {
         continue;
      }
      int work_count = epoll_wait(work->epoll_fd, work->events, BS_EPOLL_MAX_EVENTS, timeout);
      if (work_count > 0) {
         for (int index = 0; index < work_count; index++) {
            struct epoll_event event = work->events[index];
            struct client_sock *client = event.data.ptr;
            if (client) {
               if (event.events & EPOLLIN) {
                  //                size_t msg_size = client_sock_recv(client, BS_CLIENT_BUFF_SIZE, MSG_DONTWAIT);
                  DEBUG_FUNC("from: %d core: %d\n", client->sock.fd, client->current_worker->core_id);
               }

            }
         }
      }
   }
   return NULL;
}

void *epollable_thread(void *args) {
   struct server_sock *bs = (struct server_sock*)args;
   if (!bs) {
      DEBUG_ERR("no server_sock\n");
      goto free_this_trash;
   }

   int max_clients = MAX_CLIENTS_CAPABLE / CORE_COUNT;
   int extra = max_clients % CORE_COUNT;

   for (int work_index = 0; work_index < CORE_COUNT; work_index++) {
      bs->workers[work_index] = malloc(sizeof(struct worker));
      if (!bs->workers[work_index]) {
         DEBUG_ERR("no malloc worker\n");
         goto free_workers;
      }
      struct worker *work = bs->workers[work_index];
      work->epoll_fd = epoll_create1(0);
      if (work->epoll_fd < 0) {
         DEBUG_ERR("flows weren`t inited\n");
         return NULL;
      }
      work->event.events = EPOLLIN | EPOLLOUT | EPOLLET;
      work->event.data.ptr = bs;

      work->bs = bs;
      work->core_id = work_index;
      work->client_count = 0;
      work->max_clients = max_clients + (work_index < extra ? 1 : 0); 
      CHECK_MY_THREAD(goto free_workers, &work->core, NULL, bs_worker_thread, work);
   }

   int timeout = -1;
   while (!atomic_load(&bs->released)) {
      if (!atomic_load(&bs->initialized)) {
         continue;
      }

      if (atomic_load(&bs->peer_connections) >= bs->peer_capable) {
         DEBUG_ERR("not enough space!\n");
         usleep(1000);
         continue;
      }

      int ticks = epoll_wait(bs->epoll_core_fd, bs->core_events, BS_EPOLL_MAX_EVENTS, timeout);
      if (ticks > 0) {
         for (int index = 0; index < ticks; index++) {
            int client_fd = server_sock_accept(bs, NULL);
            if (client_fd > 0) {
               DEBUG_FUNC("new client accepted: %d\n", client_fd);
            }
         }
      }
   }

   DEBUG_INFO("yes yes");
   return NULL;
free_workers:
   for (int worker_index = 0; worker_index < CORE_COUNT; worker_index++) {
      struct worker *work = bs->workers[worker_index];
      worker_release(work);
   }
free_this_trash:
   return NULL;
} 

int sock_core_init() {
   int max_clients = MAX_CLIENTS_CAPABLE / CORE_COUNT;
   int extra = max_clients % CORE_COUNT;

   sock_workers = malloc(sizeof(struct worker) * CORE_COUNT);
   if (!sock_workers) {
      DEBUG_INFO("no workers\n");
      goto free_this_trash;  
   }

   for (int work_index = 0; work_index < CORE_COUNT; work_index++) {
      sock_workers[work_index] = malloc(sizeof(struct worker));
      if (!sock_workers[work_index]) {
         DEBUG_ERR("no malloc worker\n");
         goto free_everything;
      }
      struct worker *work = sock_workers[work_index];
      work->epoll_fd = epoll_create1(0);
      if (work->epoll_fd < 0) {
         DEBUG_ERR("flows weren`t inited\n");
         return NULL;
      }
      work->event.events = EPOLLIN | EPOLLOUT | EPOLLET;

      work->core_id = work_index;
      work->client_count = 0;
      work->max_clients = max_clients + (work_index < extra ? 1 : 0);
      CHECK_MY_THREAD(goto free_everything, &work->core, NULL, bs_worker_thread, work);
   }
   return 0;
free_everything:
   sock_core_exit();
free_this_trash:
   return -1;
}

void sock_core_exit() {

}

int sock_syst_init(void) {
   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
   int ret = -1;
   CORE_COUNT = (int)sysconf(_SC_NPROCESSORS_ONLN);

   ret = sock_core_init();
   if (ret < 0) {
      DEBUG_FUNC("no sock cores inited\n");
      goto free_this_trash;
   }

   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_OPEN;
   return 0;

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

   SOCKET_SYSTEM_STATUS = SOCKET_STATUS_CLOSE;
}
