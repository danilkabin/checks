#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#include "core/opium_string.h"
#include "core/opium_core.h"
#include "core/opium_pool.h"
#include "opium_string.h"

   void
opium_strlow(u_char *dst, u_char *src, size_t len)
{
   for (size_t index = 0; index < len; index++) {
      dst[index] = opium_tolower(src[index]);
   }
}

   u_char *
opium_cpystrn(u_char *dst, u_char *src, size_t len)
{
   return (u_char*)strncpy((char*)dst, (const char*)src, len);
}

   u_char *
opium_strnstr(u_char *s1, u_char *s2)
{
   return (u_char*)(strstr((char*)s1, (char*)s2)); 
}


