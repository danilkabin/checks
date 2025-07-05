#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include "epoll.h"
#include "utils.h"
#include "pool.h"
#include "sup.h"

onion_epoll_static_t *epoll_smoke;

onion_epoll_static_t *onion_get_static_by_epoll(onion_epoll_t *ep) {
   return epoll_smoke;
}

time_t onion_tick() {
   struct timespec time;
   clock_gettime(CLOCK_MONOTONIC_RAW, &time);
   return time.tv_sec;
}

int onion_fd_is_valid(int fd) {
    if (fd < 0) return -1;
    if (fcntl(fd, F_GETFD) == -1 && errno == EBADF) {
        DEBUG_ERR("Fd descriptor is invalid\n");
        return -1;
    }
    return 0;
}

void onion_epoll_tag_set(onion_epoll_tag_t *tag, int fd, onion_handler_ret_t type, void *data) {
   tag->fd = fd;
   tag->type = type;
   tag->user_data = data;
}

int onion_epoll_event_add(int fd, int set_fd, struct onion_block *tags, onion_handler_ret_t type, void *data) {
   int ret;
   onion_epoll_tag_t *tag = (onion_epoll_tag_t*)onion_block_alloc(tags, NULL);
   if (!tag) {
      DEBUG_ERR("Epoll tag initialization failed!\n");
      return -1;
   }
   onion_epoll_tag_set(tag, set_fd, type, data);

   struct epoll_event event = {.events = EPOLLIN, .data.ptr = tag};
   ret = epoll_ctl(fd, EPOLL_CTL_ADD, set_fd, &event);
   if (ret < 0) {
      DEBUG_ERR("Epoll ctl add timer failed!\n");
      onion_epoll_tag_set(tag, -1, ONION_EPOLL_HANDLER_UNKNOWN, NULL);
      return -1;
   }

   return 0;
}

void onion_epoll_event_del(int fd, struct onion_block *tags, onion_epoll_tag_t *tag) {
   if (onion_fd_is_valid(tag->fd)) {
      epoll_ctl(fd, EPOLL_CTL_DEL, tag->fd, NULL);
   }
   onion_epoll_tag_set(tag, -1, ONION_EPOLL_HANDLER_UNKNOWN, NULL);
   onion_block_free(tags, tag);
}

int onion_epoll_add_timer(onion_epoll_t *ep) {
   int ret;
   ep->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
   if (onion_fd_is_valid(ep->timerfd) == -1) {
      DEBUG_FUNC("Epoll timer failed!\n");
      return -1;
   }

   struct itimerspec timer_spec = {
      .it_interval = {ONION_TIMER_INTERVAL, 0},
      .it_value = {ONION_TIMER_INTERVAL, 0}
   };

   ret = timerfd_settime(ep->timerfd, 0, &timer_spec, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll timerfd settime failed!\n");
      return -1;
   }

   ret = onion_epoll_event_add(ep->data.epoll_fd, ep->timerfd, ep->data.tags, ONION_EPOLL_HANDLER_TIMERFD, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll tag initialization failed!\n");
      return -1;
   }

   return 0;
}

void onion_epoll_remove_timer(onion_epoll_t *ep) {
   onion_epoll_data_t *data = &ep->data;
   if (onion_fd_is_valid(ep->timerfd) == 0) {
      if (onion_fd_is_valid(data->epoll_fd) == 0) {
         epoll_ctl(data->epoll_fd, EPOLL_CTL_DEL, ep->timerfd, NULL);
      }
      close(ep->timerfd);
      ep->timerfd = -1;
   }
}

onion_handler_ret_t onion_handle_timer(onion_epoll_t *ep) {
   struct onion_block *pool = ep->slots;
   size_t offset = 0;
   int start_pos = -1;
   onion_bitmask *bitmask = ep->slots->bitmask;
   while ((start_pos = onion_ffb(bitmask, offset, 1)) != -1) {
      offset = start_pos + 1;
      onion_epoll_slot_t *slot = (onion_epoll_slot_t*)onion_block_get(pool, start_pos);
      if (slot->start_pos != start_pos) {
         continue;
      }

      // DEBUG_FUNC("from core: %d hello: %ld\n", ep->core, onion_tick() - slot->time_alive);

      time_t sleepy = onion_tick() - slot->time_alive;
      if (sleepy >= slot->time_limit) {
         DEBUG_FUNC("Anonymous is offline man!\n");
         onion_epoll_remove_slot(ep, slot);
      }
   }
   return ONION_EPOLL_HANDLER_TIMERFD;
}

onion_handler_ret_t onion_epoll_tag_handler(onion_epoll_t *ep, onion_epoll_tag_t *tag, struct epoll_event *event) {
    onion_handler_ret_t ret = ONION_EPOLL_HANDLER_UNKNOWN;

    if (onion_fd_is_valid(tag->fd) != 0) {
        return -1;
    }

    if (event->events & EPOLLIN) {
        uint64_t expirations;
        ssize_t read_bytes = read(tag->fd, &expirations, sizeof(expirations));

        if (read_bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            return -1;
        }
        if (read_bytes == 0) {
            return -1;
        }
    }

    switch (tag->type) {
        case ONION_EPOLL_HANDLER_EPFD:
            break;
        case ONION_EPOLL_HANDLER_EVENTFD:
            break;
        case ONION_EPOLL_HANDLER_TIMERFD:
            ret = onion_handle_timer(ep);
            break;
        case ONION_EPOLL_HANDLER_PUPPYFD:
            break;
        case ONION_EPOLL_HANDLER_UNKNOWN:
        default:
            ret = ONION_EPOLL_HANDLER_UNKNOWN;
            break;
    }

    return ret;
}

static int onion_epoll_handler_check_args(void *arg, onion_epoll_static_t **ep_st, onion_epoll_t **ep) {
    if (!arg) {
        DEBUG_ERR("Thread argument is NULL!\n");
        return -1;
    }
    struct onion_thread_args *thread_args = (struct onion_thread_args *)arg;
    if (!thread_args) {
        DEBUG_ERR("Thread argument structure is NULL!\n");
        return -1;
    }
    *ep_st = thread_args->ep_st;
    if (!*ep_st) {
        DEBUG_ERR("onion_epoll_static pointer is NULL!\n");
        return -1;
    }
    *ep = thread_args->ep;
    if (!*ep) {
        DEBUG_ERR("onion_epoll pointer is NULL!\n");
        return -1;
    }
    return 0;
}

static void onion_epoll_handle_event(onion_epoll_t *ep, onion_epoll_data_t *data, struct epoll_event *event) {
    onion_epoll_tag_t *tag = (onion_epoll_tag_t *)event->data.ptr;
    if (!tag) return;

    onion_handler_ret_t tag_ret = onion_epoll_tag_handler(ep, tag, event);
    if (tag_ret == ONION_EPOLL_HANDLER_UNKNOWN || tag_ret < 0) {
        onion_epoll_event_del(data->epoll_fd, data->tags, tag);
    }
}

void *onion_epoll_handler(void *arg) {
    onion_epoll_static_t *ep_st = NULL;
    onion_epoll_t *ep = NULL;

    if (onion_epoll_handler_check_args(arg, &ep_st, &ep) < 0) {
        return NULL;
    }

    onion_epoll_data_t *data = &ep->data;

    if (onion_cpu_set_core(ep->flow, ep->core) < 0) {
        DEBUG_ERR("Failed to set thread affinity inside handler, core = %d\n", ep->core);
        onion_slave_epoll1_exit(ep_st, ep);
        return NULL;
    }

    struct epoll_event events[ONION_EPOLL_PER_MAX_EVENTS];

    while (ep && ep->initialized) {
        int event_count = epoll_wait(data->epoll_fd, events, ONION_EPOLL_PER_MAX_EVENTS, 100);

        if (ep->handler) {
            struct onion_thread_my_args args = {
                .ep_st = ep_st,
                .ep = ep,
            };
            ep->handler(&args);
        }

        if (event_count <= 0) {
            continue;
        }

        for (int i = 0; i < event_count; i++) {
            onion_epoll_handle_event(ep, data, &events[i]);
        }
    }

    onion_slave_epoll1_exit(ep_st, ep);
    return NULL;
}

onion_epoll_slot_t *onion_epoll_add_slot(onion_epoll_t *ep, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data) {
   if (ep->conn_count + 1 >= ep->conn_max) {
      DEBUG_ERR("Epoll slot limit reached!\n");
      return NULL;
   }

   int start_pos = -1;
   onion_epoll_slot_t *ep_slot = onion_block_alloc(ep->slots, &start_pos);
   if (!ep_slot) {
      DEBUG_ERR("Failed to allocate epoll slot block!\n");
      goto unsuccessfull;
   }

   ep_slot->fd = fd;
   ep_slot->start_pos = start_pos;


   if (onion_epoll_event_add(ep->data.epoll_fd, fd, ep->data.tags, ONION_EPOLL_HANDLER_PUPPYFD, ep_slot) < 0) {
      DEBUG_ERR("epoll_ctl ADD failed for fd %d\n", fd);
      goto fail_slot;
   }

   ep_slot->data = data;
   ep_slot->func = func;
   ep_slot->shutdown = shutdown;
   ep_slot->shutdown_data = shutdown_data;

   ep_slot->time_alive = onion_tick();
   ep_slot->time_limit = ONION_ANONYMOUS_TIME_ALIVE;

   ep->conn_count += 1;
   atomic_store(&ep_slot->initialized, 1);

   DEBUG_FUNC("Epoll slot added (fd=%d, pos=%d)\n", fd, start_pos);

   return ep_slot;
fail_slot:
   onion_block_free(ep->slots, ep_slot);
unsuccessfull:
   return NULL;
}

void onion_epoll_remove_slot(onion_epoll_t *ep, onion_epoll_slot_t *ep_slot) {
   onion_epoll_data_t *data = &ep->data;
   if (onion_fd_is_valid(ep_slot->fd) == 0) {
      if (onion_fd_is_valid(data->epoll_fd) == 0) {
         epoll_ctl(data->epoll_fd, EPOLL_CTL_DEL, ep_slot->fd, NULL);
      }
      close(ep_slot->fd);
      ep_slot->fd = -1;
   }

   if (ep_slot->start_pos >= 0) {
      ep->conn_count -= 1;
   }

   onion_block_free(ep->slots, ep_slot);
   DEBUG_FUNC("Epoll slot removed\n");
}

void onion_epoll1_data_set(onion_epoll_data_t *data) {
   data->epoll_fd = -1;
   data->event_fd = -1;
   data->tags = NULL;
}

int onion_epoll1_init(onion_epoll_data_t *data, int EPOLL_FLAGS, int EVENT_FLAGS, size_t TAG_MAX_SIZE, size_t TAG_PER_SIZE) {
   int ret;

   ret = epoll_create1(EPOLL_FLAGS);
   if (onion_fd_is_valid(ret) == -1) {
      DEBUG_ERR("epoll_create1 failed.\n");
      goto unsuccessfull;
   }
   data->epoll_fd = ret;

   ret = eventfd(0, EVENT_FLAGS);
   if (onion_fd_is_valid(ret) == -1) {
      DEBUG_ERR("eventfd creation failed.\n");
      goto please_free;
   }
   data->event_fd = ret;

   ret = onion_block_init(&data->tags, TAG_MAX_SIZE, TAG_PER_SIZE);
   if (ret < 0) {
      DEBUG_ERR("Failed to init tag queue.\n");
      goto please_free;
   }

   ret = onion_epoll_event_add(data->epoll_fd, data->event_fd, data->tags, ONION_EPOLL_HANDLER_EVENTFD, NULL);
   if (ret < 0) {
      DEBUG_ERR("Failed to add eventfd to epoll.\n");
      goto please_free;
   }

   return 0;

please_free:
   onion_epoll1_exit(data);

unsuccessfull:
   return -1;
}

void onion_epoll1_exit(onion_epoll_data_t *data) {
   if (data->tags) {
      onion_block_exit(data->tags);
      data->tags = NULL;
   }

   if (onion_fd_is_valid(data->event_fd) >= 0) {
      if (onion_fd_is_valid(data->epoll_fd)) {
         epoll_ctl(data->epoll_fd, EPOLL_CTL_DEL, data->event_fd, NULL);
      }
      close(data->event_fd);
      data->event_fd = 0;
   }

   if (onion_fd_is_valid(data->epoll_fd) >= 0) {
      close(data->epoll_fd);
      data->epoll_fd = 0;
   }
}

onion_epoll_t *onion_slave_epoll1_init(onion_epoll_static_t *ep_st, onion_handler_t handler, long sched_core, size_t conn_max) {
   int ret;

   if (conn_max < 1 || conn_max >= INT_MAX) {
      DEBUG_ERR("Invalid conn_max: must be > 0 and < INT_MAX.\n");
      return NULL;
   }

   if (ep_st->count + 1 > ep_st->capable) {
      DEBUG_ERR("Exceeded onion_epoll_t slot limit.\n");
      return NULL;
   }

   if (!handler) {
      DEBUG_ERR("Handler must not be NULL.\n");
      return NULL;
   }

   long current_core = onion_get_offset_sched() + ep_st->count;

   onion_epoll_t *ep = onion_block_alloc(ep_st->epolls, NULL);
   if (!ep) {
      DEBUG_ERR("Failed to allocate onion_epoll_t.\n");
      goto unsuccessfull;
   }

   onion_epoll_data_t *data = &ep->data;
   size_t tag_max_size = ONION_TAG_QUEUE_CAPABLE * sizeof(onion_epoll_tag_t);
   size_t tag_per_size = sizeof(onion_epoll_tag_t);

   onion_epoll1_data_set(data);
   ep->initialized  = false;
   ep->conn_count   = 0;
   ep->conn_max     = conn_max;
   ep->handler      = handler;
   ep->core         = current_core;

   ret = onion_epoll1_init(data, EPOLL_CLOEXEC, EFD_CLOEXEC | EFD_NONBLOCK, tag_max_size, tag_per_size);
   if (ret < 0) {
      DEBUG_ERR("epoll_create1 failed.\n");
      goto please_free;
   }

   data->event.events = EPOLLIN | EPOLLOUT | EPOLLET;

   ep->args = (struct onion_thread_args *)onion_block_alloc(ep_st->epolls_args, NULL);
   if (!ep->args) {
      DEBUG_ERR("args initialization failed.\n");
      goto please_free;
   }

   ep->args->ep_st = ep_st;
   ep->args->ep    = ep;

   ret = onion_block_init(&ep->slots, ep->conn_max * sizeof(onion_epoll_slot_t), sizeof(onion_epoll_slot_t));
   if (ret < 0) {
      DEBUG_ERR("Failed to init epoll slots block.\n");
      goto please_free;
   }

   ret = pthread_create(&ep->flow, NULL, onion_epoll_handler, ep->args);
   if (ret < 0) {
      DEBUG_ERR("pthread_create failed.\n");
      goto please_free;
   }

   ret = onion_epoll_add_timer(ep);
   if (ret < 0) {
      DEBUG_ERR("Failed to integrate timer.\n");
      goto please_free;
   }

   ep_st->count += 1;
   ep->initialized = true;

   DEBUG_FUNC("Epoll initialized: fd=%d, core=%d, struct size=%zu, Current core: %ld.\n",
         data->epoll_fd, ep->core, sizeof(*ep), current_core);
   return ep;

please_free:
   onion_slave_epoll1_exit(ep_st, ep);

unsuccessfull:
   return NULL;
}

void onion_slave_epoll1_exit(onion_epoll_static_t *ep_st, onion_epoll_t *ep) {
   if (!ep)
      return;

   onion_epoll_data_t *data = &ep->data;

   if (ep->flow) {
      pthread_join(ep->flow, NULL);
      ep->flow = 0;
   }

   onion_epoll_remove_timer(ep);

   if (ep->slots) {
      onion_bitmask *bitmask = ep->slots->bitmask;
      onion_epoll_slot_t *slots = (onion_epoll_slot_t *)ep->slots->data;

      while (1) {
         int index = onion_find_bit(bitmask, 1);
         if (index == -1)
            break;

         int fd = slots[index].fd;
         if (onion_fd_is_valid(fd)) {
            onion_epoll_remove_slot(ep, &slots[index]);
         }
         onion_clear_bit(bitmask, index);
      }

      onion_block_exit(ep->slots);
      ep->slots = NULL;
   }

   onion_epoll1_exit(data);

   onion_block_free(ep_st->epolls_args, ep->args);
   onion_block_free(ep_st->epolls, ep);

   if (ep->initialized) {
      ep_st->count -= 1;
   }

   ep->initialized = false;

   DEBUG_FUNC("epoll exit\n");
}

int onion_epoll_static_init(onion_epoll_static_t **ptr, long core_count) {
    int ret;

    if (epoll_smoke) {
        DEBUG_ERR("Big smoke already existing!\n");
        return -1;
    }

    if (core_count < 1) {
        DEBUG_ERR("Core count must be at least 1. Please initialize onion_config.\n");
        return -1;
    }

    onion_epoll_static_t *ep_st = malloc(sizeof(*ep_st));
    if (!ep_st) {
        DEBUG_ERR("Failed to allocate onion_epoll_static.\n");
        return -1;
    }

    ep_st->count = 0;
    ep_st->capable = core_count;

    size_t epolls_size = sizeof(onion_epoll_t) * ep_st->capable;
    size_t epolls_args_size = sizeof(struct onion_thread_args) * ep_st->capable;

    ret = onion_block_init(&ep_st->epolls, epolls_size, sizeof(onion_epoll_t));
    if (!ep_st->epolls) {
        DEBUG_ERR("Failed to initialize epoll block (size: %zu).\n", epolls_size);
        goto unsuccessfull;
    }

    ret = onion_block_init(&ep_st->epolls_args, epolls_args_size, sizeof(struct onion_thread_args));
    if (ret < 0) {
        DEBUG_ERR("Failed to initialize epoll args (size: %zu).\n", epolls_args_size);
        goto unsuccessfull;
    }

    epoll_smoke = ep_st;
    *ptr = ep_st;

    DEBUG_FUNC("onion_epoll_static initialized (%ld cores).\n", ep_st->capable);
    return 0;

unsuccessfull:
    onion_epoll_static_exit(ep_st);
    return -1;
}

void onion_epoll_static_exit(onion_epoll_static_t *ep_st) {
    if (!ep_st)
        return;

    epoll_smoke = NULL;

    if (ep_st->epolls) {
        for (size_t index = 0; index < (size_t)ep_st->capable; ++index) {
            onion_epoll_t *epoll = (onion_epoll_t *)onion_block_get(ep_st->epolls, index);
            if (epoll->initialized) {
                onion_slave_epoll1_exit(ep_st, epoll);
            }
        }

        onion_block_exit(ep_st->epolls);
        ep_st->epolls = NULL;
    }

    if (ep_st->epolls_args) {
        onion_block_exit(ep_st->epolls_args);
        ep_st->epolls_args = NULL;
    }

    ep_st->count = 0;
    ep_st->capable = 0;

    free(ep_st);
    DEBUG_FUNC("onion_epoll_static exited.\n");
}
