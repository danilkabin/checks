#ifndef FB_HTTP 
#define FB_HTTP

#include "pool.h"
#include <sys/types.h>

#define HTTP_MAX_HEADERS  32

#define HTTP_PARSER_BUFF_SIZE 8192
#define HTTP_BODY_MAX_SIZE 8192
#define HTTP_MAX_MESSAGES 8

typedef enum {
   HTTP_MSGTYPE_RESPONSE,
   HTTP_MSGTYPE_REQUEST,
   HTTP_MSGTYPE_NULL
} http_message_type;

typedef enum {
   HTTP_PARSER_LINE,
   HTTP_PARSER_HEADER,
   HTTP_PARSER_BODY,
   HTTP_PARSER_DONE,
   HTTP_PARSER_ERROR
} http_parser_type;

typedef enum {
   HTTP_GET,
   HTTP_HEAD,
   HTTP_POST,
   HTTP_PUT,
   HTTP_DELETE,
   HTTP_CONNECT,
   HTTP_OPTIONS,
   HTTP_TRACE
} http_method;

typedef enum {
   HTTP_1_0,
   HTTP_1_1,
   HTTP_2_0
} http_version;

typedef struct {
   http_method method;
   const char *phrase;
} method_t;

typedef struct {
   u_int16_t code;
   const char *phrase;
} status_phrase_t;

static const method_t http_stringed_methods[] = {
   {HTTP_GET, "GET"},
   {HTTP_HEAD, "HEAD"},
   {HTTP_POST, "POST"},
   {HTTP_PUT, "PUT"},
   {HTTP_DELETE, "DELETE"},
   {HTTP_CONNECT, "CONNECT"},
   {HTTP_OPTIONS, "OPTIONS"},
   {HTTP_TRACE, "TRACE"},
};

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
} http_message;

typedef struct {
   http_parser_type type;
   struct memoryPool *messages;
   char buffer[HTTP_PARSER_BUFF_SIZE];
   size_t buff_length;
   size_t content_length;
   size_t content_received;
} http_parser;

#endif
