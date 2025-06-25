#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "onion.h"
#include "http.h"
#include "request.h"
#include "slab.h"

typedef struct {

   struct onion_slab *req_allocator;
   struct onion_slab *req_msg_allocator;
   onion_http_buffer_t req_buff;
   onion_http_request_t requests[ONION_HTTP_MAX_REQUESTS];
   int req_active;
   int req_pos;
   int req_head;

} onion_http_parser_t;

int http_parser_init(onion_http_parser_t *, struct onion_slab *, struct onion_slab *);
void http_parser_exit(onion_http_parser_t *);

int onion_http_parser_request(onion_http_parser_t *, char *, size_t);

#endif
