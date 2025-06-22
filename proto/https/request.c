#include "request.h"
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

   http_buffer_t *buff = &request->buff; 

   int ret = http_buff_append(buff, data, data_size);
   if (ret < 0) {
      return ret;
   } 

   request->bytes_received = request->bytes_received + data_size;

   return 0;
}

int http_parse_start_line(http_request_t *request) {
   http_start_line_t *start_line = &request->start_line;

   http_buffer_t *buff = &request->buff; 
   char *buff_data_start = http_buff_data_start(buff);
   size_t buff_data_len = http_buff_data_len(buff);

   char *endVersion = http_findStr(buff_data_start, buff_data_len, "\r\n", 2);
   if (!endVersion) {
      if (buff_data_len >= HTTP_LINE_MAX_SIZE) {
         return -1;
      }
      return 0; 
   }

   size_t line_len = endVersion - buff_data_start;

   if (line_len <= 0) {
      return -1;
   }

   char *endName = http_findChar(buff_data_start, line_len, ' ');
   if (!endName) {
      return -1;
   }

   char *urlStart = endName + 1;
   char *endUrl = http_findChar(urlStart, line_len - (urlStart - buff_data_start), ' ');
   if (!endUrl) {
      return -1;
   }

   size_t nameSize = endName - buff_data_start;
   size_t urlSize = endUrl - urlStart;

   char *versionStart = endUrl + 1;
   size_t versionSize = endVersion - versionStart;

   start_line->method = http_tok_alloc(request->line_allocator, buff_data_start, nameSize);
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

   http_buff_consume(buff, line_total);

   request->state = REQUEST_PARSE_HEADERS;
   return line_len + 2;
}

int http_parse_headers(http_request_t *request) {
   http_buffer_t *buff = &request->buff; 
   char *buff_data_start = http_buff_data_start(buff);
   size_t buff_data_len = http_buff_data_len(buff);

   char *endPoint = http_findStr(buff_data_start, buff_data_len, "\r\n\r\n", 4);
   if (!endPoint) {
      if (buff_data_len >= HTTP_MAX_HEADER_SIZE) {
         return -1;
      }
      return 0;
   }

   char *ptr = buff_data_start;
   size_t total = buff_data_len;
   size_t total_size = endPoint - buff_data_start + 4;

   for (int index = 0; index < HTTP_MAX_HEADERS + 1; index++) {
      http_header_t *header = &request->headers[index];
      char *line_end = http_findStr(ptr, total - (ptr - buff_data_start), "\r\n", 2);
      if (!line_end) {
         break;
      }
      if (line_end == ptr) {
         break;
      }
      size_t header_len = line_end - ptr;

      char *colon = http_findStr(ptr, header_len, ":", 1);
      if (!colon) {
         return -1;
      }

      size_t name_len = colon - ptr;
      size_t value_len = line_end - colon - 1;

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

      request->header_count = request->header_count + 1;

      ptr = line_end + 2;
   }

   if (request->header_count > HTTP_MAX_HEADERS) {
      return -1;
   }

   http_buff_consume(buff, total_size);
   
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

   request->line_end = 0;
   request->header_end = 0;
   request->body_end = 0;

   request->header_count = 0;
   request->bytes_received = 0;
}

int http_request_init(http_request_t *request, struct slab *allocator, struct slab *line_allocator) {
   request->allocator = allocator;
   request->line_allocator = line_allocator;

   if (!request->allocator || !request->line_allocator) {
      return -1;
   }

   int ret = http_buff_init(request->allocator, &request->buff, HTTP_LINE_MAX_SIZE, HTTP_MAX_MESSAGE_SIZE);
   if (ret < 0) {
      goto unsuccessfull;
   }

   return 0;
unsuccessfull:
   http_request_exit(request);
   return -1;
}

int http_request_reinit(http_request_t *request, struct slab *allocator, struct slab *line_allocator) {
   http_request_exit(request);
   return http_request_init(request, allocator, line_allocator);
}

void http_request_exit(http_request_t *request) {
   http_start_line_t *start_line = &request->start_line;
   http_request_reset(request);

   http_buff_exit(&request->buff);

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
   struct slab *allocator = slab_init(size);
   if (!allocator) {
      return NULL;
   }
   return allocator;
}

void http_request_device_exit(struct slab *allocator) {
   slab_exit(allocator); 
}
