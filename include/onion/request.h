/*#ifndef ONION_REQUEST_H 
#define ONION_REQUEST_H

#include "onion.h"
#include "slab.h"
#include "http.h"

#include <stdbool.h>
#include <sys/types.h>

typedef enum {
    REQUEST_PARSE_START_LINE = 0,
    REQUEST_PARSE_HEADERS,
    REQUEST_PARSE_BODY,
    REQUEST_PARSE_DONE,
    REQUEST_PARSE_ERROR
} request_parse_state_t;

typedef struct {
   onion_http_method_t method;
   char *url;
   onion_http_version_t version;
} http_start_line_t;

typedef struct {
   char *name;
   char *value;
} ontion_http_header_t;

typedef struct {
   request_parse_state_t state;
   http_start_line_t start_line;
   
   ontion_http_header_t headers[ONION_HTTP_MAX_HEADERS];
   int header_count;

   char *body;
   size_t body_len;
   size_t body_capacity;

   size_t line_end;
   size_t header_end;

   size_t bytes_received;

   bool isContentLength;
   bool isChunked;
   size_t chunk_dirty;
   size_t last_len;

   bool host;

   bool isActive;
   bool isReady;

   struct onion_slab *allocator;
} onion_http_request_t;

int onion_http_request_init(onion_http_request_t *request, struct onion_slab *);
int onion_http_request_reinit(onion_http_request_t *request, struct onion_slab *);
void onion_http_request_exit(onion_http_request_t *request);

int onion_http_request_parse(onion_http_request_t *request, onion_http_buffer_t *, char *data, size_t data_size);
int onion_http_request_append(onion_http_request_t *, size_t);

void onion_http_request_reset(onion_http_request_t *);

#endif*/
