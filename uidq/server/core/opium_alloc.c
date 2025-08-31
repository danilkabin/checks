#include "opium_alloc.h"
#include "opium_log.h"

#include <stdlib.h>
#include <string.h>

   void
opium_memzero(void *ptr, size_t size)
{
   memset(ptr, 0, size);
}

   void *
opium_malloc(size_t size, opium_log_t *log)
{
   void *ptr = malloc(size);
   if (!ptr) {
      opium_debug(log, "Malloc failed(size: %zu)\n", size);
   }

   opium_debug(log, "Allocate pointer. Size: %zu\n", size);

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
   opium_debug(log, "Pointer freed succesfully: %p, %zu\n", ptr, sizeof(ptr));
}

void *
opium_memalign(size_t alignment, size_t size, opium_log_t *log) {
   void *ptr = NULL;
   int err;

   err = posix_memalign(&ptr, alignment, size);
   if (err < 0) {
      opium_debug(log, "posix_memalign failed(size: %zu)\n", size);
      ptr = NULL;  
   }

   opium_debug(log, "posix_memalign(ptr: %p, align: %zu, size: %zu)\n",
         ptr, alignment, size);

   return ptr;
}
