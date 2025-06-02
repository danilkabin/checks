#include "lock.h"
#include "utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

int mutex_init(mutex_t *mutex, const pthread_mutexattr_t *attr) {
   int ret = -1;
   if (!mutex) {
      goto free_this_trash;
   }
   ret = pthread_mutex_init(&mutex->lock, attr);
   if (ret != 0) { 
      DEBUG_FUNC("no mutex!\n");
      goto free_this_trash;
   }
   ret = pthread_cond_init(&mutex->cond, NULL);
   if (ret != 0) {
      DEBUG_FUNC("no cond!\n");
      goto free_mutex;
   }
   atomic_store(&mutex->count, 0);
   atomic_store(&mutex->isShuttingDown, false);
   return 0;
free_mutex:
   pthread_mutex_destroy(&mutex->lock);
free_this_trash:
   return ret;
}

void mutex_destroy(mutex_t *mutex) {
   if (!mutex) {
      return;
   }
   pthread_mutex_destroy(&mutex->lock);
   pthread_cond_destroy(&mutex->cond);
   atomic_store(&mutex->count, 0);
   atomic_store(&mutex->isShuttingDown, false);
}

int mutex_lock(mutex_t *mutex) {
   if (!mutex) {
      return -1;
   }
   if (atomic_load(&mutex->isShuttingDown)) {
      return -1;
   }
   int ret = pthread_mutex_lock(&mutex->lock);
   if (ret == 0) {
      atomic_fetch_add(&mutex->count, 1);
   }
   return ret;
}

int mutex_unlock(mutex_t *mutex) {
   if (!mutex) {
      return -1;
   }
   int count = atomic_fetch_sub(&mutex->count, 1) - 1;
   int ret = pthread_mutex_unlock(&mutex->lock);
   if (count <= 0 && atomic_load(&mutex->isShuttingDown)) {
      pthread_cond_signal(&mutex->cond);
   }
   return ret;
}

void mutex_synchronise(mutex_t *mutex) {
   if (!mutex) {
      return;
   }
   pthread_mutex_lock(&mutex->lock);
   atomic_store(&mutex->isShuttingDown, true);
   while (atomic_load(&mutex->count) > 0) {
      pthread_cond_wait(&mutex->cond, &mutex->lock);
   }
   pthread_mutex_unlock(&mutex->lock);
}
