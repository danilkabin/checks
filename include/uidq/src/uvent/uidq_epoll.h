#ifndef ONION_EPOLL_H
#define ONION_EPOLL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <bits/pthreadtypes.h>

#include "core/uidq_slab.h"

typedef struct {
    int epoll_flags;
    int event_flags;
    int max_tags;
} onion_epoll_conf_t;

typedef struct {
   int index;
   int fd;
   void *user_data;
} uidq_epoll_tag_t;

typedef struct {
    int max_tags;
    int epoll_fd;
    int event_fd;
    struct epoll_event event;
    uidq_slab_t *tags;
} uidq_epoll_t;

uidq_epoll_tag_t *uidq_epoll_add(uidq_epoll_t *epoll, int fd, void *data, uint32_t events);
void uidq_epoll_del(uidq_epoll_t *epoll, uidq_epoll_tag_t *tag);

uidq_epoll_t *uidq_epoll_init(onion_epoll_conf_t *conf);
void uidq_epoll_exit(uidq_epoll_t *epoll_ctl);

#endif
