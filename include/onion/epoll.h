#ifndef ONION_EPOLL_H
#define ONION_EPOLL_H

#include "ringbuff.h"
#include "pool.h"
#include "ringbuff.h"
#include "slab.h"
#include "sup.h"
#include <bits/pthreadtypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <time.h>

#define ONION_PTHREAD_ENABLED true

#define ONION_EPOLL_PER_MAX_EVENTS 32
#define ONION_TIMER_INTERVAL 1
#define ONION_ANONYMOUS_TIME_ALIVE 3

#define ONION_EPOLL_TAG_QUEUE_CAPABLE 5
#define ONION_TAG_QUEUE_CAPABLE 10

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
   int fd;
   int start_pos;
 
   void *data;
   int (*func) (void*);
   
   void (*shutdown) (void*);
   void *shutdown_data;

   time_t time_alive;
   time_t time_limit;
} onion_epoll_slot_t;

typedef void *(*onion_handler_t) (void *ep, onion_epoll_tag_t *tag, onion_handler_ret_t ret);
typedef struct __attribute__((aligned(64))) {
   int fd;
   int eventfd;
   int core;
   int timerfd;

   pthread_t flow;
   int conn_count;
   int conn_max;

   struct epoll_event event;
   struct onion_block *slots;
   struct onion_block *tags;

   onion_bitmask *bitmask;
   onion_handler_t handler;

   bool active;
} onion_epoll_t;

static struct {
   struct onion_slab *slots;
   int current_slots;
   int max_slots;
} onion_epoll_static;

int onion_fd_is_valid(int fd);

int onion_epoll_static_init(size_t);
void onion_epoll_static_exit();

onion_epoll_t *onion_epoll1_init(onion_handler_t, size_t conn_max);
void onion_epoll1_exit(onion_epoll_t *);

int onion_epoll_slot_add(onion_epoll_t *ep, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data);
void onion_epoll_slot_del(onion_epoll_t *ep, onion_epoll_slot_t *ep_slot);

#endif
