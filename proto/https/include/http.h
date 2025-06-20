#ifndef FB_HTTP 
#define FB_HTTP

#include "pool.h"
#include "slab.h"
#include <stdbool.h>
#include <sys/types.h>

#define HTTP_MAX_HEADERS  32

#define HTTP_PARSER_BUFF_SIZE 8192 
#define HTTP_BODY_MAX_SIZE 8192
#define HTTP_CHUNK_MAX_SIZE 4096
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
   HTTP_PARSE_RET_FAILED = -2,
   HTTP_PARSE_RET_EXTRA = -1,
   HTTP_PARSE_RET_YET = 0,
   HTTP_PARSE_RET_OK = 1
} http_parse_ret;

typedef enum {
   HTTP_CHUNK_TYPE_SIZE,
   HTTP_CHUNK_TYPE_DATA,
   HTTP_CHUNK_TYPE_CRLF,
   HTTP_CHUNK_TYPE_DONE
} http_chunk_type;

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
   u_int16_t code;
   char *headers;
   char *body;
} response_line;

typedef struct {
   char *name;
   char *value;
} http_header;

typedef struct {
   http_message_type type;
   response_line response;
   request_line request;
   http_header headers[HTTP_MAX_HEADERS];
   char *body;
   size_t body_size;
   size_t current_body_size;
   bool isActive;
   bool isReady;
   bool isUpperParsed;
   bool isChunked;
} http_message;

typedef struct {
   http_parser_type type;
   http_chunk_type chunk_type;
   size_t chunkSize; 
   size_t currentChunkSize;
   size_t pos;
   
   char buff[HTTP_PARSER_BUFF_SIZE];  
   size_t buff_len;

   bool isChunked;

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

typedef int (*http_method_handler)();

http_consume_result http_parser_consume(http_parser*, const char*, size_t);
int http_parser_init(int core_count, http_parser *parser);
int http_parser_allocator_init(int);
void http_parser_allocator_exit();
int http_response_init(struct slab*, response_line*, size_t, size_t);
void http_response_exit(struct slab*, response_line*);
int http_message_init(http_parser*, size_t, http_message_type, size_t);
void http_message_exit(http_parser*, size_t);
int http_get_upper_part(http_parser*, char*, size_t);
int http_validate_start_line(http_parser*, http_message*);
int http_start_line_parse(http_parser*, http_message*);
int http_validate_header(char*, size_t, http_header*);
int http_headers_parse(http_parser*, http_message*, char*, size_t);
int http_message_static_parse(http_parser*, http_message*);
int http_message_chunk_parse(http_parser*, http_message*);
http_parse_ret http_message_body_parse(http_parser*, http_message*);
int http_request_handling(http_parser*, http_message*, http_method_handler*);
void http_set_message_type(http_parser*, size_t);
int http_check_on_host(char*, size_t);
http_method http_parse_method(const char*);
http_version http_parse_version(const char*);
int http_string_parse(char*, size_t, const char*, char*, size_t);
void reset_parser(http_parser*);
int http_find_free_message(http_parser*);

#endif
