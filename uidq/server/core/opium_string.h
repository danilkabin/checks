#ifndef OPIUM_STRING_INCLUDE_H
#define OPIUM_STRING_INCLUDE_H

#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include <sys/types.h>

typedef struct opium_string_s opium_string_t;

struct opium_string_s {
   size_t len;
   size_t cap;
};

u_char *opium_string_data(opium_string_t *string);
u_char *opium_string_end(opium_string_t *string);

void opium_string_set(opium_string_t *string, void *buff, size_t new_len, int mode);

opium_string_t *opium_string_init(size_t capacity, opium_slab_t *slab);
void opium_string_exit(opium_string_t *string, opium_slab_t *slab);

void opium_string_debug(opium_string_t *string);

void opium_string_append(opium_string_t *string, void *new_data, size_t new_len);
void opium_string_write(opium_string_t *string, void *new_data, size_t new_len);

void opium_string_trim(opium_string_t *string);

void *opium_string_memcpy(void *data, void *new, size_t len);
void *opium_string_memset(void *data, int set, size_t len);
void *opium_string_memzero(void *data, size_t len);

#define opium_string_data(s) ((u_char*)(s + 1))
#define opium_string_len(s) (s->len)
#define opium_string_cap(s) (s->cap)
#define opium_string_end(s) ((u_char*)(opium_string_data(s) + opium_string_len(s)))

#define opium_string_tolower(c) ((u_char)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c))
#define opium_string_toupper(c)   ((u_char)((c >= 'a' && c <= 'z') ? (c & ~0x20) : c))

#define opium_string_set_len(s, n) (s->len = n)

/*
   opium_str_copy(dst, src) — скопировать строку в новый буфер (через pool/slab).

   opium_str_dup(str) — дубликат строки.

   opium_str_cmp(a, b) — сравнение (memcmp).

   opium_str_casecmp(a, b) — сравнение без регистра.

   opium_str_equal(a, b) — проверка на равенство (длину + memcmp).

   Утилиты

   opium_str_trim(str) — убрать пробелы/\r\n.

   opium_str_find(str, substr) — поиск подстроки.

   opium_str_split(str, sep, array/list) — разбивка (например, HTTP-заголовки).

   opium_str_tolower / toupper.

   Числовые конверсии (твоя идея про stol)

   opium_str_to_long(str, base) (обёртка над strtol, но с len).

   opium_str_to_double(str).

   opium_str_from_long(num, buf) (обратное преобразование в opium_string_t).
   */

#endif /* OPIUM_STRING_INCLUDE_H */
