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

int onion_epoll_conf_init(onion_epoll_conf_t *epoll_conf) {
   epoll_conf->events_per_frame = epoll_conf->events_per_frame < 0 ? ONION_DEFAULT_EVENTS_PER_FRAME : epoll_conf->events_per_frame;
   epoll_conf->tag_queue_capable = epoll_conf->tag_queue_capable < 0 ? ONION_DEFAULT_TAG_QUEUE_CAPABLE : epoll_conf->tag_queue_capable;
   epoll_conf->timeout = epoll_conf->timeout < 0 ? ONION_DEFAULT_EPOLL_TIMEOUT : epoll_conf->timeout;
   epoll_conf->sec_interval = epoll_conf->sec_interval < 0 ? ONION_DEFAULT_SEC_INTERVAL : epoll_conf->sec_interval;
   epoll_conf->nsec_interval = epoll_conf->nsec_interval < 0 ? ONION_DEFAULT_NSEC_INTERVAL : epoll_conf->nsec_interval;
   epoll_conf->sec_value = epoll_conf->sec_value < 0 ? ONION_DEFAULT_SEC_VALUE : epoll_conf->sec_value;
   epoll_conf->nsec_value = epoll_conf->nsec_value < 0 ? ONION_DEFAULT_NSEC_VALUE : epoll_conf->nsec_value;
   return 0;
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

onion_epoll_static_t *onion_epoll_priv(onion_epoll_t *epoll) {
   return epoll->parent;
}

/**
 * @section onion_epoll_event
 * @brief Epoll event registration and removal helpers.
 *
 * These functions provide convenient wrappers for adding/removing file descriptors
 * to/from an epoll instance using tag-based metadata structures.
 *
 * - onion_epoll_event_add:  Allocates and associates an event tag, then adds the file descriptor to epoll.
 * - onion_epoll_event_del:  Removes the file descriptor from epoll, clears the tag, and frees its memory.
 *
 */
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

/**
 * @section onion_epoll_timer
 * @brief Timer management for epoll event loop.
 *
 * These functions provide periodic timer integration for the epoll-based event loop, enabling
 * timeouts and scheduled cleanup of inactive connections.
 *
 * - onion_epoll_add_timer:      Creates and adds a timerfd to the epoll instance for periodic events.
 * - onion_epoll_remove_timer:   Removes and closes the timerfd from the epoll instance.
 * - onion_handle_timer:         Callback to handle timer events, cleaning up expired slots.
 *
 */
int onion_epoll_add_timer(onion_epoll_t *epoll) {
   int ret;
   onion_epoll_static_t *epoll_static = onion_epoll_priv(epoll);
   if (!epoll_static) {
      DEBUG_ERR("No epoll static!\n");
      return -1;
   }
   onion_epoll_conf_t *epoll_conf = epoll_static->conf;

   epoll->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
   if (onion_fd_is_valid(epoll->timerfd) == -1) {
      DEBUG_FUNC("Epoll timer failed!\n");
      return -1;
   }

   struct itimerspec timer_spec = {
      .it_interval = {epoll_conf->sec_value, epoll_conf->nsec_value}, 
      .it_value = {epoll_conf->sec_interval, epoll_conf->nsec_interval}
   };

   ret = timerfd_settime(epoll->timerfd, 0, &timer_spec, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll timerfd settime failed!\n");
      return -1;
   }

   ret = onion_epoll_event_add(epoll->data.epoll_fd, epoll->timerfd, epoll->data.tags, ONION_EPOLL_HANDLER_TIMERFD, NULL);
   if (ret < 0) {
      DEBUG_ERR("Epoll tag initialization failed!\n");
      return -1;
   }

   return 0;
}

void onion_epoll_remove_timer(onion_epoll_t *epoll) {
   onion_epoll_data_t *data = &epoll->data;
   if (onion_fd_is_valid(epoll->timerfd) == 0) {
      if (onion_fd_is_valid(data->epoll_fd) == 0) {
         epoll_ctl(data->epoll_fd, EPOLL_CTL_DEL, epoll->timerfd, NULL);
      }
      close(epoll->timerfd);
      epoll->timerfd = -1;
   }
}

onion_handler_ret_t onion_handle_timer(onion_epoll_t *epoll) {
   struct onion_block *slots_block = epoll->slots;
   onion_bitmask *bitmask = epoll->slots->bitmask;

   size_t offset = 0;
   int pos = -1;

   while ((pos = onion_ffb(bitmask, offset, 1)) != -1) {
      offset = pos + 1;

      onion_epoll_slot_t *slot = (onion_epoll_slot_t *)onion_block_get(slots_block, pos);

      if (slot->start_pos != pos) {
         continue;
      }

      time_t alive_time = onion_tick() - slot->time_alive;
      if (alive_time >= slot->time_limit) {
         DEBUG_FUNC("Anonymous is offline man! (fd=%d, alive=%ld)\n", slot->fd, alive_time);
         onion_epoll_remove_slot(epoll, slot);
      }
   }
   return ONION_EPOLL_HANDLER_TIMERFD;
}

/**
 * @section onion_epoll_tag
 * @brief Tag-based epoll event dispatching and management.
 *
 * These functions provide a mechanism to associate metadata ("tags") with epoll events,
 * enabling flexible and type-safe dispatching of multiple event types (eventfd, timerfd, etc).
 *
 * - onion_epoll_tag_set:     Set fields of the tag structure for an epoll event.
 * - onion_epoll_tag_handler: Dispatch handler logic based on the tag's type and the incoming epoll event.
 *
 */
void onion_epoll_tag_set(onion_epoll_tag_t *tag, int fd, onion_handler_ret_t type, void *data) {
   tag->fd = fd;
   tag->type = type;
   tag->user_data = data;
}

onion_handler_ret_t onion_epoll_tag_handler(onion_epoll_t *epoll, onion_epoll_tag_t *tag, struct epoll_event *event) {
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
         ret = onion_handle_timer(epoll);
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

/**
 * @section onion_epoll_thread
 * @brief Epoll thread handler and event dispatching.
 *
 * These internal functions manage the lifecycle and event loop of an epoll worker thread.
 * They handle argument checking, and proper cleanup on shutdown.
 *
 * - onion_epoll_handler_check_args:  Validates and extracts thread arguments for an epoll worker.
 * - onion_epoll_handle_event:        Handles a single epoll event, dispatches to tag handler and cleans up on error.
 * - onion_epoll_handler:             Main thread function for the epoll worker; 
 *
 */
static int onion_epoll_handler_check_args(void *arg, onion_epoll_static_t **epoll_static, onion_epoll_t **epoll) {
   if (!arg) {
      DEBUG_ERR("Thread argument is NULL!\n");
      return -1;
   }
   struct onion_thread_args *thread_args = (struct onion_thread_args *)arg;
   if (!thread_args) {
      DEBUG_ERR("Thread argument structure is NULL!\n");
      return -1;
   }
   *epoll_static = thread_args->epoll_static;
   if (!*epoll_static) {
      DEBUG_ERR("onion_epoll_static pointer is NULL!\n");
      return -1;
   }
   *epoll = thread_args->epoll;
   if (!*epoll) {
      DEBUG_ERR("onion_epoll pointer is NULL!\n");
      return -1;
   }
   return 0;
}

static void onion_epoll_handle_event(onion_epoll_t *epoll, onion_epoll_data_t *data, struct epoll_event *event) {
   onion_epoll_tag_t *tag = (onion_epoll_tag_t *)event->data.ptr;
   if (!tag) return;

   onion_handler_ret_t tag_ret = onion_epoll_tag_handler(epoll, tag, event);
   if (tag_ret == ONION_EPOLL_HANDLER_UNKNOWN || tag_ret < 0) {
      onion_epoll_event_del(data->epoll_fd, data->tags, tag);
   }
}

void *onion_epoll_handler(void *arg) {
   onion_epoll_static_t *epoll_static = NULL;
   onion_epoll_t *epoll = NULL;

   if (onion_epoll_handler_check_args(arg, &epoll_static, &epoll) < 0) {
      return NULL;
   }

   onion_epoll_conf_t *epoll_conf = epoll_static->conf;
   onion_epoll_data_t *data = &epoll->data;

   if (onion_cpu_set_core(epoll->flow, epoll->core) < 0) {
      DEBUG_ERR("Failed to set thread affinity inside handler, core = %d\n", epoll->core);
      onion_slave_epoll1_exit(epoll_static, epoll);
      return NULL;
   }

   struct epoll_event events[epoll_conf->events_per_frame];

   while (epoll && !atomic_load(&epoll->should_stop)) {
      int event_count = epoll_wait(data->epoll_fd, events, epoll_conf->events_per_frame, 100);

      if (epoll->handler) {
         struct onion_thread_my_args args = {
            .epoll_static = epoll_static,
            .epoll = epoll,
         };
         epoll->handler(&args);
      }

      if (event_count <= 0) {
         continue;
      }

      for (int index = 0; index < event_count; index++) {
         onion_epoll_handle_event(epoll, data, &events[index]);
      }
   }

   onion_slave_epoll1_exit(epoll_static, epoll);
   return NULL;
}

/**
 * @section onion_epoll_slot_management
 * @brief Epoll slot management for dynamic file descriptor handling.
 *
 * These functions manage the allocation, initialization, and cleanup of per-connection
 * epoll slots within an epoll worker instance. Each slot represents an active file descriptor
 * and associated callbacks/state.
 *
 * - onion_epoll_add_slot:     Allocates and sets up a new epoll slot, registers it with epoll.
 * - onion_epoll_remove_slot:  Removes an epoll slot from epoll and frees all associated resources.
 *
 */
onion_epoll_slot_t *onion_epoll_add_slot(onion_epoll_t *epoll, int fd, void *data, int (*func) (void*), void (*shutdown) (void*), void *shutdown_data) {
   onion_epoll_static_t *epoll_static = onion_epoll_priv(epoll);
   if (!epoll_static) {
      DEBUG_ERR("No epoll static!\n");
      return NULL;
   }
   onion_epoll_conf_t *epoll_conf = epoll_static->conf;
   if (epoll->conn_count + 1 >= epoll->conn_max) {
      DEBUG_ERR("Epoll slot limit reached!\n");
      return NULL;
   }

   int start_pos = -1;
   onion_epoll_slot_t *ep_slot = onion_block_alloc(epoll->slots, &start_pos);
   if (!ep_slot) {
      DEBUG_ERR("Failed to allocate epoll slot block!\n");
      goto unsuccessfull;
   }

   ep_slot->fd = fd;
   ep_slot->start_pos = start_pos;


   if (onion_epoll_event_add(epoll->data.epoll_fd, fd, epoll->data.tags, ONION_EPOLL_HANDLER_PUPPYFD, ep_slot) < 0) {
      DEBUG_ERR("epoll_ctl ADD failed for fd %d\n", fd);
      goto fail_slot;
   }

   ep_slot->data = data;
   ep_slot->func = func;
   ep_slot->shutdown = shutdown;
   ep_slot->shutdown_data = shutdown_data;

   ep_slot->time_alive = onion_tick();
   ep_slot->time_limit = epoll_conf->timeout;

   epoll->conn_count = epoll->conn_count + 1;
   atomic_store(&ep_slot->initialized, 1);

   DEBUG_FUNC("Epoll slot added (fd=%d, pos=%d)\n", fd, start_pos);

   return ep_slot;
fail_slot:
   onion_block_free(epoll->slots, ep_slot);
unsuccessfull:
   return NULL;
}

void onion_epoll_remove_slot(onion_epoll_t *epoll, onion_epoll_slot_t *ep_slot) {
   onion_epoll_data_t *data = &epoll->data;
   if (onion_fd_is_valid(ep_slot->fd) == 0) {
      if (onion_fd_is_valid(data->epoll_fd) == 0) {
         epoll_ctl(data->epoll_fd, EPOLL_CTL_DEL, ep_slot->fd, NULL);
      }
      close(ep_slot->fd);
      ep_slot->fd = -1;
   }

   if (ep_slot->start_pos >= 0) {
      epoll->conn_count -= 1;
   }

   onion_block_free(epoll->slots, ep_slot);
   DEBUG_FUNC("Epoll slot removed\n");
}

/**
 * @section onion_epoll1
 * @brief General abstraction for creating and managing an epoll instance.
 *
 * This section provides a simplified interface for initializing and cleaning up
 * a single epoll structure, as well as managing its associated eventfd and tag queue.
 *
 * Main functions:
 *  - onion_epoll1_init:   Create and initialize an epoll instance with eventfd and tag queue.
 *  - onion_epoll1_exit:   Destroy the epoll instance and free all resources.
 *  - onion_epoll1_data_set: Reset epoll data structure to initial state.
 */
int onion_epoll1_init(onion_epoll_data_t *data, int EPOLL_FLAGS, int EVENT_FLAGS, size_t tag_max_count) {
   int ret;

   size_t tag_max_size = tag_max_count * sizeof(onion_epoll_tag_t);
   size_t tag_per_size = sizeof(onion_epoll_tag_t);

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

   data->tags = onion_block_init(TAG_MAX_SIZE, TAG_PER_SIZE);
   if (!data->tags) {
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

void onion_epoll1_data_set(onion_epoll_data_t *data) {
   data->epoll_fd = -1;
   data->event_fd = -1;
   data->tags = NULL;
}

/**
 * @section onion_epoll_worker
 * @brief Epoll worker lifecycle management.
 *
 * These functions are responsible for creating, initializing, running,
 * and properly cleaning up an individual epoll worker instance.
 *
 * - onion_slave_epoll1_init: Allocates, initializes, and launches an epoll worker thread.
 * - onion_slave_epoll1_exit: Cleans up, stops, and frees all resources associated with an epoll worker.
 */
onion_epoll_t *onion_slave_epoll1_init(onion_epoll_static_t *epoll_static, onion_handler_t handler, int sched_core, size_t conn_max) {
   int ret;

   onion_epoll_conf_t *epoll_conf = epoll_static->conf;

   if (conn_max < 1 || conn_max >= INT_MAX) {
      DEBUG_ERR("Invalid conn_max: must be > 0 and < INT_MAX.\n");
      return NULL;
   }

   if (epoll_static->count + 1 > epoll_static->capable) {
      DEBUG_ERR("Exceeded onion_epoll_t slot limit.\n");
      return NULL;
   }

   if (!handler) {
      DEBUG_ERR("Handler must not be NULL.\n");
      return NULL;
   }

   int curr_core = epoll_static->capable > 1 ? (epoll_static->count >= sched_core ? epoll_static->count + 1 : epoll_static->count) : epoll_static->count;
   if (curr_core >= epoll_static->capable) {
         return NULL;
   }

   onion_epoll_t *epoll = onion_block_alloc(epoll_static->epolls, NULL);
   if (!epoll) {
      DEBUG_ERR("Failed to allocate onion_epoll_t.\n");
      goto unsuccessfull;
   }

   onion_epoll_data_t *data = &epoll->data;

   onion_epoll1_data_set(data);
   epoll->initialized  = false;
   epoll->conn_count   = 0;
   epoll->conn_max     = conn_max;
   epoll->handler      = handler;
   epoll->core         = curr_core;
   epoll->parent       = epoll_static;
   atomic_store(&epoll->should_stop, false);

   ret = onion_epoll1_init(data, EPOLL_CLOEXEC, EFD_CLOEXEC | EFD_NONBLOCK, epoll_conf->tag_queue_capable);
   if (ret < 0) {
      DEBUG_ERR("epoll_create1 failed.\n");
      goto please_free;
   }

   data->event.events = EPOLLIN | EPOLLOUT | EPOLLET;

   epoll->args = (struct onion_thread_args *)onion_block_alloc(epoll_static->epolls_args, NULL);
   if (!epoll->args) {
      DEBUG_ERR("args initialization failed.\n");
      goto please_free;
   }

   epoll->args->epoll_static = epoll_static;
   epoll->args->epoll        = epoll;

   size_t size = sizeof(onion_epoll_slot_t);
   size_t capable = size * epoll->conn_max;
   DEBUG_FUNC("CPABLE EESYEYS:%zu\n", capable / size);
   epoll->slots = onion_block_init(capable, size);
   if (!epoll->slots) {
      DEBUG_ERR("Failed to init epoll slots block.\n");
      goto please_free;
   }

   ret = pthread_create(&epoll->flow, NULL, onion_epoll_handler, epoll->args);
   if (ret < 0) {
      DEBUG_ERR("pthread_create failed.\n");
      goto please_free;
   }

   ret = onion_epoll_add_timer(epoll);
   if (ret < 0) {
      DEBUG_ERR("Failed to integrate timer.\n");
      goto please_free;
   }

   epoll_static->count = epoll_static->count + 1;
   epoll->initialized = true;

   DEBUG_FUNC("Epoll initialized: fd=%d, core=%d, struct size=%zu.\n",
         data->epoll_fd, epoll->core, sizeof(*epoll));
   return epoll;

please_free:
   onion_slave_epoll1_exit(epoll_static, epoll);

unsuccessfull:
   return NULL;
}

void onion_slave_epoll1_exit(onion_epoll_static_t *epoll_static, onion_epoll_t *epoll) {
   if (!epoll) {
      return;
   }

   onion_epoll_data_t *data = &epoll->data;
  
   atomic_store(&epoll->should_stop, true);
  
   if (epoll->flow) {
      pthread_join(epoll->flow, NULL);
      epoll->flow = 0;
   }

   onion_epoll_remove_timer(epoll);

   if (epoll->slots) {
      onion_bitmask *bitmask = epoll->slots->bitmask;
      onion_epoll_slot_t *slots = (onion_epoll_slot_t *)epoll->slots->data;

      while (1) {
         int index = onion_find_bit(bitmask, 1);
         if (index == -1)
            break;

         int fd = slots[index].fd;
         if (onion_fd_is_valid(fd)) {
            onion_epoll_remove_slot(epoll, &slots[index]);
         }
         onion_clear_bit(bitmask, index);
      }

      onion_block_exit(epoll->slots);
      epoll->slots = NULL;
   }

   onion_epoll1_exit(data);

   onion_block_free(epoll_static->epolls_args, epoll->args);
   onion_block_free(epoll_static->epolls, epoll);

   if (epoll->initialized) {
      epoll_static->count -= 1;
   }

   epoll->initialized = false;

   DEBUG_FUNC("epoll exit\n");
}

/**
 * @section onion_epoll_static
 * @brief Global epoll static state management.
 *
 * These functions manage the lifecycle of the global epoll static structure, which contains
 * all epoll workers and thread argument pools for the process.
 *
 * - onion_epoll_static_init:  Allocates and initializes the global static state for epoll workers.
 * - onion_epoll_static_exit:  Cleans up and frees all resources associated with the global static state.
 */
onion_epoll_static_t *onion_epoll_static_init(onion_epoll_conf_t *epoll_conf, int core_count) {
   int ret;

   if (core_count < 1) {
      DEBUG_ERR("Core count must be at least 1. Please initialize onion_config.\n");
      return NULL;
   }

   onion_epoll_static_t *epoll_static = malloc(sizeof(*epoll_static));
   if (!epoll_static) {
      DEBUG_ERR("Failed to allocate onion_epoll_static.\n");
      return NULL;
   }

   epoll_static->conf = epoll_conf;
   epoll_static->count = 0;
   epoll_static->capable = core_count;

   size_t epolls_size = sizeof(onion_epoll_t) * epoll_static->capable;
   size_t epolls_args_size = sizeof(struct onion_thread_args) * epoll_static->capable;

   epoll_static->epolls = onion_block_init(epolls_size, sizeof(onion_epoll_t));
   if (!epoll_static->epolls) {
      DEBUG_ERR("Failed to initialize epoll block (size: %zu).\n", epolls_size);
      goto unsuccessfull;
   }

   epoll_static->epolls_args = onion_block_init(epolls_args_size, sizeof(struct onion_thread_args));
   if (!epoll_static->epolls_args) {
      DEBUG_ERR("Failed to initialize epoll args (size: %zu).\n", epolls_args_size);
      goto unsuccessfull;
   }

   DEBUG_FUNC("onion_epoll_static initialized (%ld cores).\n", epoll_static->capable);
   return epoll_static;
unsuccessfull:
   onion_epoll_static_exit(epoll_static);
   return NULL;
}

void onion_epoll_static_exit(onion_epoll_static_t *epoll_static) {
   if (!epoll_static) {
      return;
   }

   if (epoll_static->epolls) {
      for (size_t index = 0; index < (size_t)epoll_static->capable; ++index) {
         onion_epoll_t *epoll = (onion_epoll_t *)onion_block_get(epoll_static->epolls, index);
         if (epoll->initialized) {
            onion_slave_epoll1_exit(epoll_static, epoll);
         }
      }

      onion_block_exit(epoll_static->epolls);
      epoll_static->epolls = NULL;
   }

   if (epoll_static->epolls_args) {
      onion_block_exit(epoll_static->epolls_args);
      epoll_static->epolls_args = NULL;
   }

   epoll_static->count = 0;
   epoll_static->capable = 0;

   free(epoll_static);
   DEBUG_FUNC("onion_epoll_static exited.\n");
}
