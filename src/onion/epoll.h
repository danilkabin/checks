#ifndef ONION_EPOLL_H
#define ONION_EPOLL_H

#include "pool.h"
#include "slab.h"
#include "sup.h"
#include <bits/pthreadtypes.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <time.h>

#define ONION_PTHREAD_ENABLED true

#define ONION_TIMER_INTERVAL 1

typedef void *(*onion_handler_t) (void *);

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

typedef struct {
   int fd;
   int eventfd;
   int core;
   int timerfd;

   pthread_t flow;
   int conn_count;
   int conn_max;

   struct epoll_event event;
   struct onion_block *slots;

   onion_bitmask bitmask;
   onion_handler_t handler;

   bool active;
} onion_epoll_t;

static struct {
   int current_slots;
   int max_slots;
   struct onion_slab *slots;
} onion_epoll_static;

int onion_epoll_static_init(size_t);
void onion_epoll_static_exit();

onion_epoll_t *onion_epoll1_init(onion_handler_t, size_t conn_max);
void onion_epoll1_exit(onion_epoll_t *);

int onion_epoll_slot_add(onion_epoll_t *ep, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data);
void onion_epoll_slot_del(onion_epoll_t *ep, onion_epoll_slot_t *ep_slot);

#endif
