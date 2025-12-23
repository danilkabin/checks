/* @file opium_alloc.c
 * @brief Memory allocation wrappers for the Opium core library
 *
 * This module provides thin wrappers around standard C allocation
 * functions (malloc, calloc, free, posix_memalign), with integrated
 * logging support. It ensures that every memory operation can be 
 * tracked and debugged consistently.
 *
 */

#include "core/opium_core.h"

   void *
opium_malloc(size_t size, opium_log_t *log)
{
   void *ptr = malloc(size);
   if (!ptr) {
      opium_log_debug(log, "Malloc failed(size: %zu)\n", size);
   }

   opium_log_debug(log, "Allocate pointer. Size: %zu\n", size);

   return ptr;
}

   void *
opium_calloc(size_t size, opium_log_t *log)
{
   void *ptr = opium_malloc(size, log);

   if (ptr) {
      opium_memzero(ptr, size);
   }

   return ptr;
}

   void
opium_free(void *ptr, opium_log_t *log)
{
   if (!ptr) return;
   free(ptr);
   (void)log;
   //opium_log_debug(log, "Pointer freed succesfully: %p, %zu\n", ptr, sizeof(ptr));
}

void *
opium_mmap(size_t size, opium_log_t *log)
{
   void *ptr = NULL;
   
   ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   if (!ptr) {
      opium_log_debug(log, "mmap failed(size: %zu)\n", size);
      ptr = NULL;  
   }

   return ptr;
}

void
opium_munmap(void *data, size_t size, opium_log_t *log) {
   if (munmap(data, size) == -1) {
      opium_log_debug(log, "munmap failed(size: %zu)\n", size);
   }
}

void *
opium_memalign(size_t alignment, size_t size, opium_log_t *log) {
   void *ptr = NULL;
   int err;

   err = posix_memalign(&ptr, alignment, size);
   if (err < 0) {
      opium_log_debug(log, "posix_memalign failed(size: %zu)\n", size);
      ptr = NULL;  
   }

 //  opium_log_debug(log, "posix_memalign(ptr: %p, align: %zu, size: %zu)\n",
 //        ptr, alignment, size);

   return ptr;
}

   void *
opium_memcpy(void *dst, void *src, size_t len)
{
   if (len == 0) {
      return NULL;
   }

   return memcpy(dst, src, len);
}
