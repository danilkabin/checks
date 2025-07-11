#ifndef ONION_EPOLL_H
#define ONION_EPOLL_H

#include "pool.h"

#include <bits/pthreadtypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <time.h>

#define ONION_DEFAULT_EVENTS_PER_FRAME 32
#define ONION_DEFAULT_TAG_QUEUE_CAPABLE 10
#define ONION_DEFAULT_EPOLL_TIMEOUT 5
#define ONION_DEFAULT_SEC_INTERVAL 1
#define ONION_DEFAULT_NSEC_INTERVAL 0
#define ONION_DEFAULT_SEC_VALUE 1
#define ONION_DEFAULT_NSEC_VALUE 0

typedef struct {
   int events_per_frame;
   int tag_queue_capable;
   int timeout;
   int sec_interval;
   int nsec_interval;
   int sec_value;
   int nsec_value;
} onion_epoll_conf_t;

struct onion_thread_args;
struct onion_thread_my_args;

typedef enum {
   ONION_EPOLL_HANDLER_EPFD = 4308,
   ONION_EPOLL_HANDLER_EVENTFD,
   ONION_EPOLL_HANDLER_TIMERFD,
   ONION_EPOLL_HANDLER_PUPPYFD,
   ONION_EPOLL_HANDLER_UNKNOWN
} onion_handler_ret_t;

typedef struct {
   onion_handler_ret_t type;
   int fd;
   void *user_data;
} onion_epoll_tag_t;

typedef struct {
   int epoll_fd;
   int event_fd;
   struct epoll_event event; 
   struct onion_block *tags;
} onion_epoll_data_t;

typedef struct {
   int fd;
   int start_pos;
 
   void *data;
   int (*func) (void*);
   
   void (*shutdown) (void*);
   void *shutdown_data;

   time_t time_alive;
   time_t time_limit;
   atomic_int initialized;
} onion_epoll_slot_t;

typedef void *(*onion_handler_t) (struct onion_thread_my_args *args);

typedef struct {
   onion_epoll_conf_t *conf;
   struct onion_block *epolls;
   struct onion_block *epolls_args;
   long capable;
   long count;
} onion_epoll_static_t;

typedef struct {
   onion_epoll_static_t *parent;
   onion_epoll_data_t data;
   int core;
   int timerfd;

   pthread_t flow;
   atomic_bool should_stop;
   int conn_count;
   int conn_max;

   struct onion_block *slots;
   struct onion_thread_args *args;

   onion_handler_t handler;

   bool initialized;
} onion_epoll_t;

struct onion_thread_args {
   onion_epoll_static_t *epoll_static;
   onion_epoll_t *epoll;
};

struct onion_thread_my_args {
   onion_epoll_static_t *epoll_static;
   onion_epoll_t *epoll;
};

int onion_epoll_conf_init(onion_epoll_conf_t *epoll_conf);
int onion_fd_is_valid(int fd);
void onion_epoll_tag_set(onion_epoll_tag_t *tag, int fd, onion_handler_ret_t type, void *data);

onion_epoll_static_t *onion_epoll_priv(onion_epoll_t *epoll);

onion_epoll_static_t *onion_epoll_static_init(onion_epoll_conf_t *epoll_conf, int core_count);
void onion_epoll_static_exit(onion_epoll_static_t *ep_st);

int onion_epoll1_init(onion_epoll_data_t *data, int EPOLL_FLAGS, int EVENT_FLAGS, size_t TAG_MAX_SIZE, size_t TAG_PER_SIZE);
void onion_epoll1_exit(onion_epoll_data_t *data);

onion_epoll_t *onion_slave_epoll1_init(onion_epoll_static_t *ep_st, onion_handler_t, int sched_core, size_t conn_max);
void onion_slave_epoll1_exit(onion_epoll_static_t *ep_st, onion_epoll_t *);

onion_epoll_slot_t *onion_epoll_add_slot(onion_epoll_t *ep, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data);
void onion_epoll_remove_slot(onion_epoll_t *ep, onion_epoll_slot_t *ep_slot);

#endif
