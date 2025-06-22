#ifndef FB_HTTP 
#define FB_HTTP

#include <stdbool.h>
#include <sys/types.h>

#define HTTP_LINE_METHOD_MAX_SIZE 32
#define HTTP_LINE_METHOD_MAX_URL 464
#define HTTP_LINE_METHOD_MAX_VERSION 16
#define HTTP_LINE_MAX_SIZE HTTP_LINE_METHOD_MAX_SIZE + HTTP_LINE_METHOD_MAX_URL + HTTP_LINE_METHOD_MAX_VERSION

#define HTTP_MAX_HEADERS  32
#define HTTP_MAX_HEADER_SIZE 8192
#define HTTP_INITIAL_HEADER_SIZE 2048

#define HTTP_MAX_BODY_SIZE 8192
#define HTTP_INITIAL_BODY_SIZE 2048

#define HTTP_INITIAL_MESSAGE_SIZE HTTP_LINE_MAX_SIZE + HTTP_INITIAL_HEADER_SIZE + HTTP_INITIAL_BODY_SIZE
#define HTTP_MAX_MESSAGE_SIZE HTTP_LINE_MAX_SIZE + HTTP_MAX_HEADER_SIZE + HTTP_MAX_BODY_SIZE

#define HTTP_HEADER_NAME_SIZE 64
#define HTTP_HEADER_VALUE_SIZE 1024

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

   char *buff;
   size_t buff_len;
   size_t buff_capacity;

   size_t line_size;
   size_t header_size;
   size_t body_size;

   size_t sum_capacity;

   size_t bytes_received;

   struct slab *allocator;
   struct slab *line_allocator;
} http_request_t;

int http_request_init(http_request_t *request, struct slab *allocator, struct slab *line_allocator);
void http_request_exit(http_request_t *request);

struct slab *http_request_device_init(size_t size);
void http_request_device_exit(struct slab *allocator);

int http_request_parse(http_request_t *request, char *data, size_t data_size);

#endif
