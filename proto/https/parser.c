#include "parser.h"
#include "slab.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

char *http_findChar(char *start, size_t size, char target) {
   for (size_t offset = 0; offset < size; offset++) {
      if (start[offset] == target) {
         return &start[offset];
      }
   }
   return NULL;
}

char *http_findStr(char *start, size_t size, char *target, size_t target_size) {
   if (target_size <= 0 || size < target_size) {
      return NULL;
   }
   for (size_t offset = 0; offset <= size - target_size; offset++) {
      if (memcmp(&start[offset], target, target_size) == 0) {
         return &start[offset];
      }
   }
   return NULL;
}

char *http_tok_alloc(struct slab *allocator, char *src, size_t len) {
   char *ret = slab_malloc(allocator, NULL, len + 1);
   if (!ret) {
      return NULL;
   }
   memcpy(ret, src, len);
   ret[len] = '\0';
   return ret;
}

int http_request_append(http_request_t *request, char *data, size_t data_size) {
   if (request->state == REQUEST_PARSE_ERROR) {
      return -1;
   }

   size_t required = request->bytes_received + data_size;

   if (required > request->buff_capacity) {
      size_t new_capacity = request->buff_capacity;
      while (new_capacity < required) {
         new_capacity = new_capacity * 2;
      }
      if (new_capacity > HTTP_MAX_MESSAGE_SIZE) {
         return -1;
      }

      request->buff = slab_realloc(request->allocator, request->buff, new_capacity);
      if (!request->buff) {
         return -1;
      }
      DEBUG_FUNC("NEWNENWEWNEWNN\n");
      request->buff_capacity = new_capacity; 
   }

   memcpy(request->buff + request->buff_len, data, data_size);
   request->buff_len = request->buff_len + data_size;
   request->bytes_received = request->bytes_received + data_size;

   return 0;
}

int http_parse_start_line(http_request_t *request) {
DEBUG_FUNC("GOT!\n");
   http_start_line_t *start_line = &request->start_line;

   char *endVersion = http_findStr(request->buff, request->buff_len, "\r\n", 2);
   if (!endVersion) {
      if (request->buff_len >= HTTP_LINE_MAX_SIZE) {
         return -1;
      }
      return 0; 
   }

DEBUG_FUNC("NEXT!\n");
   size_t line_len = endVersion - request->buff;

   if (line_len <= 0) {
      return -1;
   }

   char *endName = http_findChar(request->buff, line_len, ' ');
   if (!endName) {
      return -1;
   }

   char *urlStart = endName + 1;
   char *endUrl = http_findChar(urlStart, line_len - (urlStart - request->buff), ' ');
   if (!endUrl) {
      return -1;
   }

   size_t nameSize = endName - request->buff;
   size_t urlSize = endUrl - urlStart;

   char *versionStart = endUrl + 1;
   size_t versionSize = endVersion - versionStart;

   start_line->method = http_tok_alloc(request->line_allocator, request->buff, nameSize);
   if (!start_line->method) {
      return -1;
   }

   start_line->url = http_tok_alloc(request->line_allocator, urlStart, urlSize);
   if (!start_line->url) {
      return -1;
   }

   start_line->version = http_tok_alloc(request->line_allocator, versionStart, versionSize);
   if (!start_line->version) {
      return -1;
   }

   size_t line_total = line_len + 2;
   
   printf("method: %s, url: %s, version: %s\n", request->start_line.method, request->start_line.url, request->start_line.version);
   
   memmove(request->buff, request->buff + line_total, request->buff_len - line_total);
   request->buff_len = request->buff_len - line_total;

   request->state = REQUEST_PARSE_HEADERS;
   return line_len + 2;
}

int http_parse_headers(http_request_t *request) {
   char *endPoint = http_findStr(request->buff, request->buff_len, "\r\n\r\n", 4);
   if (!endPoint) {
      if (request->buff_len >= HTTP_MAX_HEADER_SIZE) {
         return -1;
      }
      return 0;
   }

   char *ptr = request->buff;
   size_t total = request->buff_len;
   size_t total_size = endPoint - request->buff + 4;

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      char *line_end = http_findStr(ptr, total - (ptr - request->buff), "\r\n", 2);
      if (!line_end) {
         break;
      }
      if (line_end == ptr) {
         memmove(request->buff, request->buff + total_size, request->buff_len - total_size);
         request->buff_len = request->buff_len - total_size;
         break;
      }
      size_t header_len = line_end - ptr;

      char *colon = http_findStr(ptr, header_len, ":", 1);
      if (!colon) {
         return -1;
      }

      size_t name_len = colon - ptr;
      size_t value_len = line_end - colon + 1;

      char name[name_len];
      memcpy(name, ptr, name_len);

      char value[value_len];
      memcpy(value, ptr + name_len, value_len);

      if (name_len >= HTTP_HEADER_NAME_SIZE || value_len >= HTTP_HEADER_VALUE_SIZE) {
         printf("Big size\n");
         return -1;
      }

      header->name = http_tok_alloc(request->line_allocator, name, name_len);
      if (!header->name) {
         printf("No malloc!\n");
         return -1;
      }

      char *val_start = colon + 1;
      while (*val_start == ' ') {
         val_start++;
      }

      header->value = http_tok_alloc(request->line_allocator, val_start, line_end - val_start);
      if (!header->value) {
         printf("No malloc!\n");
         slab_free(request->line_allocator, header->name);
         header->name = NULL;
         return -1;
      }

      ptr = line_end + 2;
   }

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      if (header->value == NULL) {
         break;
      }

      printf("%s:%s\n", header->name, header->value);
   }

   request->state = REQUEST_PARSE_BODY;
   return 0;
}

int http_request_parse(http_request_t *request, char *data, size_t data_size) { 
   int ret = -1;
   int write = http_request_append(request, data, data_size);
   if (write < 0) {
      http_request_init(request, request->allocator, request->line_allocator);
      return -1;
   }
   while (1) {
      switch (request->state) {
         case REQUEST_PARSE_START_LINE:
            ret = http_parse_start_line(request);
            if (ret < 0) {
               http_request_exit(request);  
               return -1;
            }
            if (ret == 0) {
               return 0;
            }
            break;
         case REQUEST_PARSE_HEADERS:
            ret = http_parse_headers(request);
            if (ret < 0) {
               http_request_exit(request);  
               return -1;
            }
            if (ret == 0) {
               return 0;
            }
            break;
         case REQUEST_PARSE_BODY:

            break;
         case REQUEST_PARSE_DONE:
            request->state = REQUEST_PARSE_START_LINE;
            break;
         case REQUEST_PARSE_ERROR:
         default:
            break;
      }
   }

   return 0;
}

void http_request_reset(http_request_t *request) {
   request->state = REQUEST_PARSE_DONE;

   request->buff_len = 0;
   request->buff_capacity = 0;

   request->line_size = 0;
   request->header_size = 0;
   request->body_size = 0;

   request->header_count = 0;
   request->bytes_received = 0;
}

int http_request_init(http_request_t *request, struct slab *allocator, struct slab *line_allocator) {
   http_request_exit(request);

   request->allocator = allocator;
   request->line_allocator = line_allocator;

   if (!request->allocator || !request->line_allocator) {
      return -1;
   }

   request->buff = slab_malloc(allocator, NULL, HTTP_LINE_MAX_SIZE);
   if (!request->buff) {
      return -1;
   }
   request->buff_capacity = HTTP_LINE_MAX_SIZE;
   return 0;
}

void http_request_exit(http_request_t *request) {
   http_start_line_t *start_line = &request->start_line;
   http_request_reset(request);

   if (request->buff) {
      slab_free(request->allocator, request->buff);
      request->buff = NULL;
   }

   if (start_line->method) {
      slab_free(request->line_allocator, start_line->method);
      start_line->method = NULL;
   }

   if (start_line->url) {
      slab_free(request->line_allocator, start_line->url);
      start_line->url = NULL;
   }

   if (start_line->version) {
      slab_free(request->line_allocator, start_line->version);
      start_line->version = NULL;
   }

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      if (header->name) {
         slab_free(request->line_allocator, header->name);
         header->name = NULL;
      }
      if (header->value) {
         slab_free(request->line_allocator, header->value);
         header->value = NULL;
      }
   }
}

struct slab *http_request_device_init(size_t size) {
   if (size < 0) {
      return NULL;
   }
   struct slab *allocator = slab_init(size);
   if (!allocator) {
      return NULL;
   }
   return allocator;
}

void http_request_device_exit(struct slab *allocator) {
   slab_exit(allocator); 
}
