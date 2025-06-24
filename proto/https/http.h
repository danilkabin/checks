#ifndef HTTP_H
#define HTTP_H

#define HTTP_LINE_METHOD_MAX_SIZE     32
#define HTTP_LINE_URL_MAX_SIZE       464
#define HTTP_LINE_VERSION_MAX_SIZE    16

#define HTTP_LINE_MAX_SIZE          (HTTP_LINE_METHOD_MAX_SIZE + HTTP_LINE_URL_MAX_SIZE + HTTP_LINE_VERSION_MAX_SIZE)

#define HTTP_HEADER_NAME_SIZE        64
#define HTTP_HEADER_VALUE_SIZE       128
#define HTTP_MAX_HEADERS             24

#define HTTP_MAX_HEADER_SIZE        (HTTP_MAX_HEADERS * (HTTP_HEADER_NAME_SIZE + HTTP_HEADER_VALUE_SIZE))
#define HTTP_INITIAL_HEADER_SIZE    (HTTP_MAX_HEADER_SIZE / 8)

#define HTTP_INITIAL_BODY_SIZE      256
#define HTTP_MAX_BODY_SIZE          8192

#define HTTP_INITIAL_MESSAGE_SIZE  (HTTP_LINE_MAX_SIZE + HTTP_INITIAL_HEADER_SIZE + HTTP_INITIAL_BODY_SIZE)
#define HTTP_MAX_MESSAGE_SIZE      (HTTP_LINE_MAX_SIZE + HTTP_MAX_HEADER_SIZE + HTTP_MAX_BODY_SIZE)

#define HTTP_MAX_REQUESTS             8
#define HTTP_MAX_REQUESTS_SIZE      (HTTP_MAX_REQUESTS * HTTP_MAX_MESSAGE_SIZE)

#include "slab.h"

#include <stddef.h>

struct http_choice {
   char *name;
   long token;
};

typedef enum {
   HTTP_METHOD_GET = 0,
   HTTP_METHOD_POST,
   HTTP_METHOD_PUT,
   HTTP_METHOD_DELETE,
   HTTP_METHOD_HEAD,
   HTTP_METHOD_OPTIONS,
   HTTP_METHOD_PATCH,
   HTTP_METHOD_INVALID
} http_method_t;

typedef enum {
   HTTP_VERSION_1_1 = 0,
   HTTP_VERSION_INVALID = -1
} http_version_t;

typedef enum {
   HTTP_PARSER_REQUEST_LIMIT,
   HTTP_PARSER_REQUEST_ERROR,
   HTTP_PARSER_REQUEST_OK
} http_parser_request_type;

typedef struct {
   char *data;
   size_t limit;
   size_t capacity;
   size_t start;
   size_t end;
   struct slab *allocator;
} http_buffer_t;

char *http_findChar(char *, size_t, char);
char *http_findStr(char *, size_t, char *, size_t);

int http_buff_append(http_buffer_t *, char *, size_t);
int http_buff_data_len(http_buffer_t *);
char *http_buff_data_start(http_buffer_t *);
void http_buff_consume(http_buffer_t *, size_t);

int http_buff_init(struct slab *, http_buffer_t *, size_t, size_t);
int http_buff_reinit(struct slab *, http_buffer_t *, size_t, size_t);
void http_buff_exit(http_buffer_t *);

struct slab *http_slab_memory_init(size_t size);
void http_slab_memory_exit(struct slab *allocator);

int http_check_header(const char *name, const char *target);
int http_get_valid_method(const char *name, size_t size);
int http_get_valid_version(const char *name, size_t size);

#endif
