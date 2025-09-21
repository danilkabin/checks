#ifndef OPIUM_STRING_INCLUDE_H
#define OPIUM_STRING_INCLUDE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "core/opium_core.h"
#include "core/opium_alloc.h"

typedef struct {
   size_t len;
   u_char *data;
} opium_str_t;

#define opium_string(str) { sizeof(str) - 1, (u_char*) str}
#define opium_null_string {0, NULL}

#define opium_string_set(str, text) \
   (str)->len = sizeof(text) - 1;   \
   (str)->data = (u_char*) text;

#define opium_string_null(str) \
   (str)->len = 0;             \
   (str)->data = NULL;

#define opium_tolower(c) (u_char) (((c) >= 'A' && (c) <= 'Z') ? ((c) | 0x20) : (c))
#define opium_toupper(c) (u_char) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~0x20) : (c))

#define opium_strncmp(s1, s2, n) strncmp((const char *)s1, (const char *)s2, n)
#define opium_strlen(str) strlen((const char*)(str))

void opium_strlow(u_char *dst, u_char *src, size_t len);
u_char *opium_cpystrn(u_char *dst, u_char *src, size_t len);
u_char *opium_strnstr(u_char *s1, u_char *s2);

#endif /* OPIUM_STRING_INCLUDE_H */
