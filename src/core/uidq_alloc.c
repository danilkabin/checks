#include "uidq_alloc.h"
#include "uidq_utils.h"
#include <stdlib.h>

void *uidq_malloc(size_t size) {
   void *p = malloc(size);
   
   if (p == NULL) {
      DEBUG_ERR("Failed to allocate malloc.\n");
   }
   
   DEBUG_FUNC("malloc: %p, size:%zu\n", p, size);
   
   return p;
}

void *uidq_calloc(size_t size) {
   void *p = uidq_malloc(size);
   
   if (p) {
      uidq_memzero(p, size);
   }

   return p;
}

void uidq_free(void *ptr) {
   free(ptr);
}
