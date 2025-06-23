#include "request.h"
#include "http.h"
#include "parser.h"

#include "slab.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNKED "chunk"

char *http_tok_alloc(struct slab *allocator, char *src, size_t len) {
   char *ret = slab_malloc(allocator, NULL, len + 1);
   if (!ret) {
      return NULL;
   }
   memcpy(ret, src, len);
   ret[len] = '\0';
   return ret;
}

int http_request_append(http_request_t *request, size_t data_size) {
   request->bytes_received = request->bytes_received + data_size;
   return 0;
}

int http_parse_start_line(http_request_t *request, http_buffer_t *buff) {
   http_start_line_t *start_line = &request->start_line;

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

   start_line->method = http_tok_alloc(request->allocator, buff_data_start, nameSize);
   if (!start_line->method) {
      return -1;
   }

   start_line->url = http_tok_alloc(request->allocator, urlStart, urlSize);
   if (!start_line->url) {
      return -1;
   }

   start_line->version = http_tok_alloc(request->allocator, versionStart, versionSize);
   if (!start_line->version) {
      return -1;
   }

   size_t line_total = line_len + 2;

   printf("method: %s, url: %s, version: %s\n", request->start_line.method, request->start_line.url, request->start_line.version);

   http_buff_consume(buff, line_total);

   request->state = REQUEST_PARSE_HEADERS;
   return line_len + 2;
}

int http_parse_headers(http_request_t *request, http_buffer_t *buff) {
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

      if (name_len >= HTTP_HEADER_NAME_SIZE || value_len >= HTTP_HEADER_VALUE_SIZE) {
         printf("Big size\n");
         return -1;
      }

      header->name = http_tok_alloc(request->allocator, ptr, name_len);
      if (!header->name) {
         printf("No malloc!\n");
         return -1;
      }

      char *val_start = colon + 1;
      while (*val_start == ' ') {
         val_start++;
      }

      header->value = http_tok_alloc(request->allocator, val_start, line_end - val_start);
      if (!header->value) {
         printf("No malloc!\n");
         slab_free(request->allocator, header->name);
         header->name = NULL;
         return -1;
      }

      const char *transferEncoding = "Transfer-Encoding";
      const char *contentLength = "Content-Length";

      if (strncmp(header->name, transferEncoding, strlen(transferEncoding)) == 0) {
         if (strncmp(header->value, CHUNKED, strlen(CHUNKED)) == 0) {
            request->isChunked = true;
         }
      }

      if (!request->isChunked) {
         if (strncmp(header->name, contentLength, strlen(contentLength)) == 0) {
            request->content_len = atoi(header->value);
         }
      }

      request->header_count = request->header_count + 1;

      ptr = line_end + 2;
   }

   if (request->header_count > HTTP_MAX_HEADERS) {
      return -1;
   }

   http_buff_consume(buff, total_size);

   printf("CHUNKED: %d Content-Length: %d\n", request->isChunked, request->content_len);

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

int http_transfer_encoding_handle(http_request_t *request, http_buffer_t *buff, size_t body_size) {
   DEBUG_FUNC("body size: %zu\n", body_size);
   return 0;
}

int http_parse_body(http_request_t *request, http_buffer_t *buff) {
   int ret = -1;
   size_t body_size = http_buff_data_len(buff);
  /* if (body_size >= HTTP_MAX_BODY_SIZE) {
      return ret;
   }*/

   if (request->isChunked) {
      return http_transfer_encoding_handle(request, buff, body_size);
   }
   return ret;
}

int http_request_parse(http_request_t *request, http_buffer_t *buff, char *data, size_t data_size) { 
   int ret = -1;
   while (1) {
      switch (request->state) {
         case REQUEST_PARSE_START_LINE:
            ret = http_parse_start_line(request, buff);
            if (ret <= 0) {
               return ret;
            }
            break;
         case REQUEST_PARSE_HEADERS:
            ret = http_parse_headers(request, buff);
            if (ret <= 0) {
               return ret;
            }
            break;
         case REQUEST_PARSE_BODY:
            ret = http_parse_body(request, buff);
            if (ret <= 0) {
               return ret;
            }
            printf("heello i am body!\n");
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
   
   request->header_count = 0;

   request->line_end = 0;
   request->header_end = 0;
   request->body_end = 0;

   request->sum_capacity = 0;
   request->bytes_received = 0;

   request->content_len = 0;
   request->isChunked = false;

   request->isActive = false;
   request->isReady = false;
}

int http_request_init(http_request_t *request, struct slab *allocator) {
   http_request_reset(request);

   request->allocator = allocator;

   request->isActive = true;
   return 0;
}

int http_request_reinit(http_request_t *request, struct slab *allocator) {
   http_request_exit(request);
   return http_request_init(request, allocator);
}

void http_request_exit(http_request_t *request) {
   http_start_line_t *start_line = &request->start_line;
   http_request_reset(request);

   if (start_line->method) {
      slab_free(request->allocator, start_line->method);
      start_line->method = NULL;
   }

   if (start_line->url) {
      slab_free(request->allocator, start_line->url);
      start_line->url = NULL;
   }

   if (start_line->version) {
      slab_free(request->allocator, start_line->version);
      start_line->version = NULL;
   }

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      if (header->name) {
         slab_free(request->allocator, header->name);
         header->name = NULL;
      }
      if (header->value) {
         slab_free(request->allocator, header->value);
         header->value = NULL;
      }
   }
}
