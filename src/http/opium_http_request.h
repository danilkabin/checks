#ifndef OPIUM_HTTP_REQUEST_INCLUDE_H
#define OPIUM_HTTP_REQUEST_INCLUDE_H

#include "core/opium_core.h"
#include "http/opium_http.h"

/* Status codes:
 *
 * 1xx [Informational] : The request was received, continuing process
 * 2xx [Successfull]   : The request was successfully received, understood, and accepted
 * 3xx [Redirection]   : Further action needs to be taken in order to complete the request
 * 4xx [Client Error]  : The request contains bad syntax or cannot be fulfilled 
 * 5xx [Servee Error]  : The server failed to fulfill an appartently valid request
 *
 */

#define OPIUM_HTTP_CONTINUE               100
#define OPIUM_HTTP_SWITCHING_PROTOCOLS    101
#define OPIUM_HTTP_HTTP_PROCESSING        102
#define OPIUM_HTTP_EARLY_HINTS            103

#define OPIUM_HTTP_OK                     200
#define OPIUM_HTTP_CREATED                201
#define OPIUM_HTTP_ACCEPTED               202
#define OPIUM_HTTP_NO_CONTENT             204
#define OPIUM_HTTP_RESET_CONTENT          205
#define OPIUM_HTTP_PARTIAL_CONTENT        206

/*
 *
 */
#define OPIUM_HTTP_MULTIPLE_CHOICES       300
#define OPIUM_HTTP_MOVED_PERMANENTLY      301
#define OPIUM_HTTP_FOUND                  302
#define OPIUM_HTTP_SEE_OTHER              303
#define OPIUM_HTTP_NOT_MODIFIED           304
#define OPIUM_HTTP_USE_PROXY              305
#define OPIUM_HTTP_TEMPORARY_REDIRECTED   307
#define OPIUM_HTTP_PERMANENT_REDIRECTED   308

/*
 *
 */
#define OPIUM_HTTP_BAD_REQUEST            400
#define OPIUM_HTTP_UNAUTHORIZED           401
#define OPIUM_HTTP_PAYMENT_REQUIRED       402
#define OPIUM_HTTP_FORBIDDEN              403
#define OPIUM_HTTP_NOT_FOUND              404
#define OPIUM_HTTP_METHOD_NOT_ALLOWED     405
#define OPIUM_HTTP_NOT_ACCEPTABLE         406
#define OPIUM_HTTP_REQUEST_TIMEOUT        408
#define OPIUM_HTTP_CONFLICT               409
#define OPIUM_HTTP_LENGTH_REQUIRED        411
#define OPIUM_HTTP_PRECONDITION_FAILED    412
#define OPIUM_HTTP_CONTENT_TOO_LARGE      413
#define OPIUM_HTTP_URI_TOO_LONG           414
#define OPIUM_HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define OPIUM_HTTP_RANGE_NOT_SATISFIABLE  416
#define OPIUM_HTTP_MISDIRECTED_REQUEST    421
#define OPIUM_HTTP_TOO_MANY_REQUESTS      429

/*
 *
 */
#define OPIUM_HTTP_INTERNAL_SERVER_ERROR  500
#define OPIUM_HTTP_NOT_IMPLEMENTED        501
#define OPIUM_HTTP_BAD_GATEWAY            502
#define OPIUM_HTTP_SERVICE_UNAVAILABLE    503
#define OPIUM_HTTP_GATEWAY_TIMEOUT        504
#define OPIUM_HTTP_VERSION_NOT_SUPPORTED  505

typedef struct {
   opium_string_t               name; 
   opium_uint_t                 offset;
   opium_http_request_handler_t handler; 
} opium_http_header_t;

typedef struct {

} opium_http_headers_in_t;

typedef struct {

} opium_http_headers_out_t;

struct opium_http_request_s {


   opium_http_headers_in_t  headers_in;
   opium_http_headers_out_t headers_out;

   opium_arena_t *arena;
};

#endif
