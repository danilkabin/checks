#include "uidq_alloc.h"
#include "uidq_log.h"

#include <stdlib.h>
#include <string.h>

   void
uidq_memzero(void *ptr, size_t size) 
{
   memset(ptr, 0, size);
}

   void *
uidq_malloc(size_t size, uidq_log_t *log)
{
   void *ptr = malloc(size);
   if (!ptr) {
      uidq_debug(log, "Malloc failed(size: %zu)\n", size);
   }

   uidq_debug(log, "Allocate pointer. Size: %zu\n", size);

   return ptr;
}

   void *
uidq_calloc(size_t size, uidq_log_t *log)
{
   void *ptr = uidq_malloc(size, log);

   if (ptr) {
      uidq_memzero(ptr, size);
   }

   return ptr;
}

   void
uidq_free(void *ptr, uidq_log_t *log)
{
   if (!ptr) return;
   free(ptr);
   uidq_debug(log, "Pointer freed succesfully: %p, %zu\n", ptr, sizeof(ptr));
}

void *
uidq_memalign(size_t alignment, size_t size, uidq_log_t *log) {
   void *ptr;
   int err;

   err = posix_memalign(&ptr, alignment, size);
   if (err < 0) {
      uidq_debug(log, "posix_memalign failed(size: %zu)\n", size);
      ptr = NULL;  
   }

   uidq_debug(log, "posix_memalign(ptr: %p, align: %zu, size: %zu)\n",
         ptr, alignment, size);

   return ptr;
}
