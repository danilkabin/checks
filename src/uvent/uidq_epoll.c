#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "core/uidq_slab.h"
#include "core/uidq_alloc.h"
#include "core/uidq_utils.h"
#include "uvent/uidq_epoll.h"

size_t tag_size = sizeof(uidq_epoll_tag_t);

int epoll_flags_default = EPOLLIN | EPOLLOUT | EPOLLET;
int event_flags_default = EFD_CLOEXEC;

uidq_slab_t *uidq_epoll_tags_init(size_t max_tags) {
   if (max_tags < 1) {
      return NULL; 
   }

   size_t tag_max_tag_size = tag_size * max_tags;

   uidq_slab_t *tags = uidq_slab_create_and_init(tag_max_tag_size, tag_size);
   if (!tags) {
      DEBUG_ERR("Failed to allocate epoll tags.\n");
      return NULL; 
   }
   uidq_memzero(tags->data, tag_max_tag_size);

   return tags;
}

void uidq_epoll_tags_exit(uidq_slab_t *tags) {
   if (!tags || !tags->initialized) {
      return;
   }
   uidq_slab_exit(tags);
}

uidq_epoll_tag_t *uidq_epoll_add(uidq_epoll_t *epoll, int fd, void *data, uint32_t events) {
   uidq_slab_t *tags = epoll->tags;
   if (!tags) {
      DEBUG_ERR("Failed to find tags.\n");   
      return NULL;
   }

   int index = uidq_slab_alloc(epoll->tags, NULL, tag_size);
   if (index < 0) {
      DEBUG_ERR("Failed to allocate epoll tag.\n");
      return NULL;
   }

   uidq_epoll_tag_t *tag = uidq_slab_get(tags, index);
   uidq_memzero(tag, tag_size); 
   tag->index = index;
   tag->fd = fd;
   tag->user_data = data;

   struct epoll_event eventix = {0};
   eventix.events = events;
   eventix.data.ptr = tag;

   if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_ADD, fd, &eventix) < 0) {
      DEBUG_ERR("Failed to epoll_ctl_add.\n");
      goto free_tag;
   }

   return tag;
free_tag:
   uidq_epoll_del(epoll, tag);
   return NULL;
}

void uidq_epoll_del(uidq_epoll_t *epoll, uidq_epoll_tag_t *tag) {
   if (!epoll || !tag) {
      return;
   }
   uidq_slab_t *slab = epoll->tags;

   tag->user_data = NULL;
   if (tag->fd >= 0) {
      epoll_ctl(epoll->epoll_fd, EPOLL_CTL_DEL, tag->fd, NULL);
      tag->fd = -1;
   }
   if (tag->index >= 0) {
      uidq_slab_dealloc(slab, tag->index);
      tag->index = -1;
   }
}

uidq_epoll_t *uidq_epoll_init(onion_epoll_conf_t *conf) {
   if (!conf || conf->max_tags < 1) {
      DEBUG_ERR("Invalid configuration parameters.\n");
      return NULL;
   }

   int epoll_flags = conf->epoll_flags ? conf->epoll_flags : epoll_flags_default;
   int event_flags = conf->event_flags ? conf->event_flags : event_flags_default;
   int max_tags = conf->max_tags;

   uidq_epoll_t *epoll_ctl = uidq_calloc(sizeof(uidq_epoll_t));
   if (!epoll_ctl) {
      DEBUG_ERR("Failed to allocate epoll struct.\n");
      return NULL;     
   }

   epoll_ctl->max_tags = max_tags;
   epoll_ctl->epoll_fd = -1;
   epoll_ctl->event_fd = -1;
   epoll_ctl->tags = NULL;

   epoll_ctl->epoll_fd = epoll_create1(epoll_flags);
   if (epoll_ctl->epoll_fd == -1) {
      DEBUG_ERR("Failed to allocate epoll fd.\n");
      goto free_ctl;
   }

   epoll_ctl->event_fd = eventfd(0, event_flags);
   if (epoll_ctl->event_fd == -1) {
      DEBUG_ERR("Failed to allocate event fd.\n");
      goto free_ctl;
   }

   epoll_ctl->tags = uidq_epoll_tags_init(max_tags);
   if (!epoll_ctl->tags) {
      DEBUG_ERR("Failed to find tags.\n");
      goto free_ctl;
   }

   return epoll_ctl;
free_ctl:
   uidq_epoll_exit(epoll_ctl);
   return NULL;
}

void uidq_epoll_exit(uidq_epoll_t *epoll_ctl) {
   if (!epoll_ctl) {
      return;
   }

   if (epoll_ctl->tags) {
      uidq_slab_exit(epoll_ctl->tags);
      epoll_ctl->tags = NULL;
   }
   if (epoll_ctl->event_fd != -1) {
      close(epoll_ctl->event_fd);
      epoll_ctl->event_fd = -1;
   }
   if (epoll_ctl->epoll_fd != -1) {
      close(epoll_ctl->epoll_fd);
      epoll_ctl->epoll_fd = -1;
   }

   free(epoll_ctl);
}
