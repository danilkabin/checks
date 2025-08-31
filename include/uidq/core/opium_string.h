#ifndef OPIUM_STRING_INCLUDE_H
#define OPIUM_STRING_INCLUDE_H

#include <sys/types.h>

typedef struct opium_string_s opium_string_t;

struct opium_string_s {
   u_char *data;
   size_t len;
};

#define opium_str_null ((opium_string_t){.ptr = NULL, .len = 0})

#define opium_string_set(s, buf, len) \
   (s)->data = buf;                   \
   (s)->len  = len;

#define opium_string_data(s) ((s)->data)
#define opium_string_len(s)  ((s)->len)
#define opium_string_end(s)  ((s)->data + (s)->len)

#define opium_string_is_empty(s) ((s)->len == 0)

#endif /* OPIUM_STRING_INCLUDE_H */
