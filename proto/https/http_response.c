#include "http.h"
#include "slab.h"

int http_response_init(struct slab *allocator, response_line *response, size_t body_size, size_t headers_size) {
   response->body = slab_malloc(allocator, NULL, body_size);
   if (!response->body) {
      return -1;
   }
   response->headers = slab_malloc(allocator, NULL, headers_size);
   if (!response->headers) {
      slab_free(allocator, response->body);
      return -1;
   }
   return 0;
}

void http_response_exit(struct slab *allocator, response_line *response) {
   if (response->body) {
      slab_free(allocator, response->body);
      response->body = NULL;
   }
   if (response->headers) {
      slab_free(allocator, response->headers);
      response->headers = NULL;
   }
}

