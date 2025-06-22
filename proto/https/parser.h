#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "slab.h"

#include <stddef.h>

typedef struct {
   char *data;
   size_t limit;
   size_t capacity;
   size_t start;
   size_t end;
   struct slab *allocator;
} http_buffer_t;

int http_buff_append(http_buffer_t *, char *, size_t);
int http_buff_data_len(http_buffer_t *);
char *http_buff_data_start(http_buffer_t *);
void http_buff_consume(http_buffer_t *, size_t);

int http_buff_init(struct slab *, http_buffer_t *, size_t, size_t);
int http_buff_reinit(struct slab *, http_buffer_t *, size_t, size_t);
void http_buff_exit(http_buffer_t *);

#endif
