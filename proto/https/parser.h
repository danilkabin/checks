#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http.h"
#include "request.h"
#include "slab.h"

typedef struct {

   struct slab *req_allocator;
   struct slab *req_msg_allocator;
   http_buffer_t req_buff;
   http_request_t requests[HTTP_MAX_REQUESTS];
   int req_active;
   int req_pos;
   int req_head;

} http_parser_t;

int http_parser_init(http_parser_t *, struct slab *, struct slab *);
void http_parser_exit(http_parser_t *);

http_parser_request_type http_parser_request(http_parser_t *, char *, size_t);

#endif
