#include "opium_string.h"
#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include "opium_core.h"
#include "core/opium.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define OPIUM_STRING_REPLACE 1
#define OPIUM_STRING_APPEND  2

   int
opium_string_is_valid(opium_string_t *string)
{
   return string && string->cap > 0;
}

   void 
opium_string_set(opium_string_t *string, void *buff, size_t new_len, int mode)
{
   if (!string || new_len == 0) {
      return;
   }

   u_char *data = opium_string_data(string);
   size_t len = opium_string_len(string);
   size_t cap = opium_string_cap(string);

   switch (mode) {
      case OPIUM_STRING_REPLACE:
         opium_string_memzero(data, cap);
         if (buff) opium_string_memcpy(data, buff, new_len);
         string->len = new_len;
         break;

      case OPIUM_STRING_APPEND:
         if (len + new_len > cap) return;
         if (buff) opium_string_memcpy(data + len, buff, new_len);
         string->len = len + new_len;
         break;

      default:
         return;
   }

   opium_string_debug(string);
}

   opium_string_t *
opium_string_init(size_t capacity, opium_slab_t *slab)
{
   if (!slab || capacity == 0) {
      return NULL;
   }

   size_t size = sizeof(opium_string_t) + capacity;

   opium_string_t *string = opium_slab_push(slab, size);
   if (!string) {
      return NULL;
   }

   u_char *data = opium_string_data(string);
   size_t cap = opium_string_cap(string);

   string->len = 0;
   string->cap = capacity;
   opium_string_memzero(data, cap);

   return string; 
}

   void
opium_string_exit(opium_string_t *string, opium_slab_t *slab)
{
   if (!slab) {
      return;
   }

   opium_slab_pop(slab, string);
}

void
opium_strlow()
{

}

   void
opium_string_write(opium_string_t *string, void *new_data, size_t new_len)
{
   opium_string_set(string, new_data, new_len, OPIUM_STRING_REPLACE);
}

   void
opium_string_append(opium_string_t *string, void *new_data, size_t new_len)
{
   opium_string_set(string, new_data, new_len, OPIUM_STRING_APPEND);
}

   void
opium_string_trim(opium_string_t *string)
{
   if (!opium_string_is_valid(string)) {
      return;
   }

   size_t len = opium_string_len(string);
   size_t cap = opium_string_len(string);
   u_char *data = opium_string_data(string);

   size_t pos = 0;
   while (pos < len) {
      pos++;
   }
}

   void *
opium_string_memcpy(void *data, void *new, size_t len)
{
   if (len >= OPIUM_MEMCPY_LIMIT) {
      opium_debug_point(); 
   }

   return memcpy(data, new, len);
}

   void *
opium_string_memset(void *data, int set, size_t len)
{
   if (len >= OPIUM_MEMSET_LIMIT) {
      opium_debug_point();
   }

   return memset(data, set, len);
}

   void *
opium_string_memzero(void *data, size_t len)
{
   return opium_string_memset(data, 0, len);
}

   void
opium_string_debug(opium_string_t *string)
{
   if (!opium_string_is_valid(string)) {
      return;
   }

   u_char *data = opium_string_data(string);

   fwrite(data, 1, string->len, stdout);
   fwrite("\n", 1, 1, stdout);
}
