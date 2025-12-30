/* opium_mutex.c 
 *
 * ERRORCHECK type is used in debug builds to detect:
 *  - Double locks by the same thread (deadlocks)
 *  - Unlocks by threads that do not own the mutex
 */

#include "core/opium_core.h"

   opium_s32_t
opium_thread_mutex_init(opium_mutex_t *mtx, opium_log_t *log)
{
   opium_err_t    err;
   pthread_mutexattr_t attr;

   err = pthread_mutexattr_init(&attr);
   if (err != 0) {
      opium_log_err(log, "pthread_mutexattr_init() failed\n");
      return OPIUM_RET_ERR;
   }

   /*
    * ERRORCHECK mutex:
    *   - Detects attempts by the same thread to re-lock (returns EDEADLK)
    *   - Detects unlocking by a thread that does not own the mutex (returns EPERM)
    *   - Safer for debugging and library-level locks
    */

   /* ATTR cleanup: Always destroy the attribute object after init attempt */
   err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
   if (err != 0) {
      opium_log_err(log, "pthread_mutexattr_settype() failed\n");
      pthread_mutexattr_destroy(&attr);
      return OPIUM_RET_ERR;
   }

   err = pthread_mutex_init(mtx, &attr);
   if (err != 0) {
      opium_log_err(log, "pthread_mutex_init() failed\n");
      pthread_mutexattr_destroy(&attr); 
      return OPIUM_RET_ERR;
   }

   /*
    * Once the mutex is initialized, the attribute object is no longer needed.
    * Not destroying it would leak internal memory/resources.
    * Even if destroy fails, the mutex itself is valid and can be used.
    */
   err = pthread_mutexattr_destroy(&attr);
   if (err != 0) {
      opium_log_err(log, "pthread_mutexattr_destroy() failed\n");
   }

   return OPIUM_RET_OK;
}

   opium_s32_t
opium_thread_mutex_exit(opium_mutex_t *mtx, opium_log_t *log)
{
   opium_err_t err;

   /*
    * The mutex must **not** be locked by any thread when destroyed.
    * Destroying a locked mutex results in EBUSY (pthread-defined behavior).
    */
   err = pthread_mutex_destroy(mtx);
   if (err != 0) {
      opium_log_err(log, "pthread_mutex_destroy() failed\n");
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   opium_s32_t
opium_thread_mutex_lock(opium_mutex_t *mtx, opium_log_t *log)
{
   opium_err_t err;

   /*
    * If the mutex is ERRORCHECK (as in our create function):
    *   - Locking the mutex twice from the same thread returns EDEADLK
    *   - Locking a destroyed or invalid mutex returns EINVAL
    */
   err = pthread_mutex_lock(mtx);
   if (err != 0) {
      opium_log_err(log, "pthread_mutex_lock() failed\n");
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}

   opium_s32_t
opium_thread_mutex_unlock(opium_mutex_t *mtx, opium_log_t *log)
{
   opium_err_t err;

   /*
    * If the mutex is ERRORCHECK:
    *   - Unlocking a mutex not owned by the current thread returns EPERM
    *   - Unlocking a destroyed or invalid mutex returns EINVAL
    */
   err = pthread_mutex_unlock(mtx);
   if (err != 0) {
      opium_log_err(log, "pthread_mutex_unlock() failed\n");
      return OPIUM_RET_ERR;
   }

   return OPIUM_RET_OK;
}
