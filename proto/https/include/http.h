#ifndef FB_HTTP 
#define FB_HTTP

#include "pool.h"
#include "slab.h"
#include <stdbool.h>
#include <sys/types.h>

#define HTTP_MAX_HEADERS  32

#define HTTP_PARSER_BUFF_SIZE 8192 
#define HTTP_BODY_MAX_SIZE 8192
#define HTTP_MAX_MESSAGES 8

#define HTTP_HEADER_NAME_SIZE 64
#define HTTP_HEADER_VALUE_SIZE 1024

#define HTTP_CHECK_HOST true

#define HTTP_METHOD_LIST \
   X(GET)     \
   X(HEAD)    \
   X(POST)    \
   X(PUT)     \
   X(DELETE)  \
   X(CONNECT) \
   X(OPTIONS) \
   X(TRACE)   \

#define HTTP_VERSION_LIST \
   V(VERSION_1_0, "HTTP/1.0")     \
   V(VERSION_1_1, "HTTP/1.1")     \
   V(VERSION_2_0, "HTTP/2.0")     \

typedef enum {
   HTTP_CONSUME_OK = 0,
   HTTP_CONSUME_BAD_CORE = 1,
   HTTP_CONSUME_NO_ALLOC = 2,
   HTTP_CONSUME_PARSE_FAIL = 3
} http_consume_result;

typedef enum {
   HTTP_MSGTYPE_RESPONSE,
   HTTP_MSGTYPE_REQUEST,
   HTTP_MSGTYPE_NULL
} http_message_type;

typedef enum {
   HTTP_PARSER_LINE = 0,
   HTTP_PARSER_HEADER = 1,
   HTTP_PARSER_BODY = 2,
   HTTP_PARSER_DONE = 3,
   HTTP_PARSER_ERROR = 4
} http_parser_type;

typedef enum {
#define X(name) HTTP_METHOD_##name,
   HTTP_METHOD_LIST
#undef X
   HTTP_METHOD_UNKNOWN
} http_method;

typedef enum {
#define V(name, str) name,
    HTTP_VERSION_LIST
#undef V
    HTTP_VERSION_UNKNOWN
} http_version;

extern const char *http_method_m[];
extern const char *http_version_m[];

typedef struct {
   http_method method;
   const char *phrase;
} method_t;

typedef struct {
   u_int16_t code;
   const char *phrase;
} status_phrase_t;

static const status_phrase_t http_phrases[] = {
   {100, "Continue"},
   {200, "OK"},
   {201, "Created"},
   {400, "Bad Request"},
   {401, "Unauthorized"},
   {403, "Forbidden"},
   {404, "Not Found"},
   {503, "Service Unavailable"}
};

typedef struct {
   http_method method;
   char *target;
   http_version version;
} request_line;

typedef struct {
   http_version version;
   u_int16_t code;
} status_line;

typedef struct {
   char *name;
   char *value;
} http_header;

typedef struct {
   http_message_type type;
   union {
      status_line status;
      request_line request;
   } start_line;
   http_header headers[HTTP_MAX_HEADERS];
   char *body;
   size_t body_size;
   size_t current_body_size;
   bool isActive;
   bool isReady;
} http_message;

typedef struct {
   http_parser_type type;

   char buff[HTTP_PARSER_BUFF_SIZE];  
   size_t buff_len;

   int line_end;
   int headers_end;
   int body_end;

   int current_message_index;

   size_t bytes_received;

   http_message messages[HTTP_MAX_MESSAGES];
   size_t messages_count;
   size_t messages_capacity;
   struct slab *allocator;
} http_parser;

void http_message_extract(char*, size_t, size_t*);
void http_message_delete(char*, size_t*, size_t);

http_consume_result http_parser_consume(http_parser*, const char*, size_t);

int http_parser_init(int core_count, http_parser *parser);

int http_parser_allocator_init(int);
void http_parser_allocator_exit();

#endif
