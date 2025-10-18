#ifndef OPIUM_HTTP_INCLUDE_H
#define OPIUM_HTTP_INCLUDE_H

#include "core/opium_core.h"

typedef int (*opium_http_request_handler_t) ();

/*
 * opium_http_status_t
 *
 * Structure representing the HTTP response status.
 * Stores the HTTP version, status code, and pointers to
 * the status line in a buffer.
 *
 * Fields:
 *   http_version - HTTP version (e.g., 1001 = HTTP/1.1)
 *   code         - HTTP status code (e.g., 200, 404, 500)
 *   count        - auxiliary counter (for internal use)
 *   start        - pointer to the start of the status line in the buffer (e.g., "200 OK")
 *   end          - pointer to the end of the status line in the buffer
 *
 * Example HTTP status line:
 *   HTTP/1.1 200 OK\r\n
 *
 * Field mapping for this example:
 *   http_version = 1001  (OPIUM_HTTP_VERSION_11)
 *   code         = 200
 *   start        -> points to the beginning of "200 OK" in the buffer
 *   end          -> points to the end of "200 OK" (before \r\n)
 */
typedef struct {
   opium_uint_t http_version;
   opium_uint_t code;
   opium_uint_t count;

   u_char *start; 
   u_char *end; 
} opium_http_status_t;

#endif /* OPIUM_HTTP_INCLUDE_H */ 
