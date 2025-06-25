#ifndef LOCK_H
#define LOCK_H

#include "stdbool.h"

#include <pthread.h>
#include <stdatomic.h>
#include "urcu.h"

typedef struct {
   pthread_mutex_t lock;
   pthread_cond_t cond;
   atomic_int count;
   atomic_bool isShuttingDown;
} mutex_t;

typedef struct {
   pthread_t flow;
   mutex_t mutex;
   pthread_cond_t cond;
   bool isRunning;
   bool isShouldStop;
} thread_m;

int mutex_init(mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
void mutex_destroy(mutex_t *mutex);

int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);

void mutex_synchronise(mutex_t *mutex);

#endif
