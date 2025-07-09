#ifndef ONION_HTTP_H
#define ONION_HTTP_H

#include "slab.h"

#include <stddef.h>

#define ONION_DEFAULT_METHOD_SIZE 32
#define ONION_DEFAULT_URL_SIZE 464
#define ONION_DEFAULT_VERSION_SIZE 16
#define ONION_DEFAULT_HEADER_NAME_SIZE 64
#define ONION_DEFAULT_HEADER_VALUE_SIZE 128
#define ONION_DEFAULT_MAX_HEADERS 24
#define ONION_DEFAULT_INIT_BODY_SIZE 256
#define ONION_DEFAULT_MAX_BODY_SIZE 8192
#define ONION_DEFAULT_MAX_REQUESTS 8

typedef struct {
    size_t method_size;
    size_t url_size;
    size_t version_size;

    size_t header_name_size;
    size_t header_value_size;
    size_t max_headers;

    size_t init_body_size;
    size_t max_body_size;

    int max_requests;
} onion_http_conf_t;

struct onion_http_choice {
   char *name;
   long token;
};

typedef enum {
   ONION_HTTP_METHOD_GET = 0,
   ONION_HTTP_METHOD_POST,
   ONION_HTTP_METHOD_PUT,
   ONION_HTTP_METHOD_DELETE,
   ONION_HTTP_METHOD_HEAD,
   ONION_HTTP_METHOD_OPTIONS,
   ONION_HTTP_METHOD_PATCH,
   ONION_HTTP_METHOD_INVALID
} onion_http_method_t;

typedef enum {
   ONION_HTTP_VERSION_1_1 = 0,
   ONION_HTTP_VERSION_INVALID = -1
} onion_http_version_t;

extern struct onion_http_choice ONION_HTTP_METHOD_LIST[];
extern struct onion_http_choice ONION_HTTP_VERSION_LIST[];

typedef enum {
   ONION_HTTP_PARSER_REQUEST_LIMIT,
   ONION_HTTP_PARSER_REQUEST_ERROR,
   ONION_HTTP_PARSER_REQUEST_OK
} onion_http_parser_request_type;

typedef struct {
   char *data;
   size_t limit;
   size_t capacity;
   size_t start;
   size_t end;
   struct onion_slab *allocator;
} onion_http_buffer_t;

char *onion_chacha(char *, size_t, char);
char *onion_stringa(char *, size_t, char *, size_t);

int onion_http_buff_append(onion_http_buffer_t *, char *, size_t);
int onion_http_buff_data_len(onion_http_buffer_t *);
char *onion_http_buff_data_start(onion_http_buffer_t *);
void onion_http_buff_consume(onion_http_buffer_t *, size_t);

int onion_http_buff_init(struct onion_slab *, onion_http_buffer_t *, size_t, size_t);
int onion_http_buff_reinit(struct onion_slab *, onion_http_buffer_t *, size_t, size_t);
void onion_http_buff_exit(onion_http_buffer_t *);

struct onion_slab *onion_slab_memory_init(size_t size);
void onion_slab_memory_exit(struct onion_slab *allocator);

int onion_http_check_in_range(const char *data, size_t size, unsigned char min, unsigned char max, unsigned char *exp, size_t size_exp);
int onion_http_get_valid_header_name(char *data, size_t size);

int onion_http_get_valid_method(const char *name, size_t size);
int onion_http_get_valid_version(const char *name, size_t size);

#endif
