#include "core/opium_core.h"

   static void *
opium_thread_wrapper(void *arg)
{
   opium_thread_t *thread = (opium_thread_t*)arg;
   if (thread->func) {
      thread->func(thread->ctx);
   }

   opium_thread_mutex_lock(&thread->mtx, thread->log);
   thread->done = 1;
   opium_thread_mutex_unlock(&thread->mtx, thread->log);

   return NULL;
}

   opium_s32_t 
opium_thread_init(opium_thread_t *thread, opium_thread_cb cb, void *ctx, opium_log_t *log)
{
   opium_err_t    err;
   pthread_attr_t attr;

   thread->func  = cb;
   thread->ctx   = ctx;
   thread->start = thread->stop = thread->done = 0;
   thread->log   = log;

   err = opium_thread_mutex_init(&thread->mtx, log);
   if (err != OPIUM_RET_OK) {
      return OPIUM_RET_ERR;
   }

   err = pthread_create(&thread->tid, NULL, opium_thread_wrapper, thread);
   if (err != 0) {
      opium_log_err(log, "pthread_create() failed\n");
      opium_thread_mutex_exit(&thread->mtx, log);
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   opium_s32_t 
opium_thread_exit(opium_thread_t *thread, opium_log_t *log)
{
   opium_err_t    err;

   opium_thread_mutex_lock(&thread->mtx, log);
   thread->stop = 1;
   opium_thread_mutex_unlock(&thread->mtx, log);

   err = pthread_join(thread->tid, NULL); 
   if (err != 0) {
      opium_log_err(log, "pthread_join() failed\n");
   }

   err = opium_thread_mutex_exit(&thread->mtx, log);
   if (err != 0) {
      return OPIUM_RET_ERR;
   }

   thread->func  = NULL;
   thread->ctx   = NULL;
   thread->start = thread->stop = thread->done = 0;
   thread->log   = NULL;

   return OPIUM_RET_OK;
}
