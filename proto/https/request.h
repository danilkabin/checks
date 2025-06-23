#ifndef FB_HTTP 
#define FB_HTTP

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
   char *method;
   char *url;
   char *version;
} http_start_line_t;

typedef struct {
   char *name;
   char *value;
} http_header_t;

typedef struct {
   request_parse_state_t state;
   http_start_line_t start_line;
   
   http_header_t headers[HTTP_MAX_HEADERS];
   int header_count;

   char *body;
   size_t body_len;
   size_t body_capacity;

   size_t line_end;
   size_t header_end;
   size_t body_end;

   size_t sum_capacity;
   size_t bytes_received;

   bool isChunked;
   size_t chunk_dirty;

   bool isActive;
   bool isReady;

   struct slab *allocator;
} http_request_t;

int http_request_init(http_request_t *request, struct slab *);
int http_request_reinit(http_request_t *request, struct slab *);
void http_request_exit(http_request_t *request);

int http_request_parse(http_request_t *request, http_buffer_t *, char *data, size_t data_size);
int http_request_append(http_request_t *, size_t);


#endif
