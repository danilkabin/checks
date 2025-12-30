#ifndef OPIUM_THREAD_INCLUDE_H
#define OPIUM_THREAD_INCLUDE_H 

#include "core/opium_core.h"

typedef pthread_mutex_t opium_mutex_t;

opium_s32_t opium_thread_mutex_init(opium_mutex_t *mtx, opium_log_t *log);
opium_s32_t opium_thread_mutex_exit(opium_mutex_t *mtx, opium_log_t *log);
opium_s32_t opium_thread_mutex_lock(opium_mutex_t *mtx, opium_log_t *log);
opium_s32_t opium_thread_mutex_unlock(opium_mutex_t *mtx, opium_log_t *log);

typedef void *(*opium_thread_cb)(void *data);

struct opium_thread_s {
   pthread_t       tid;
   opium_thread_cb func;
   void           *ctx;

   opium_mutex_t   mtx;

   opium_u32_t    core;

   unsigned        start:1;
   unsigned        stop:1;
   unsigned        done:1;

   opium_log_t    *log;
};

opium_s32_t opium_thread_init(opium_thread_t *thread, opium_thread_cb cb, void *ctx, opium_log_t *log);
opium_s32_t opium_thread_exit(opium_thread_t *thread, opium_log_t *log);

#endif /* OPIUM_THREAD_INCLUDE_H */
