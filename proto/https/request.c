#include "request.h"
#include "http.h"
#include "parser.h"

#include "slab.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNKED "chunked"

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

void http_debug_request_stats(http_request_t *request) {
   printf("\n====================[ HTTP REQUEST ]====================\n");
   printf("[ Start Line ]\n");
   printf("  Method : %d\n", request->start_line.method);
   printf("  URL    : %s\n", request->start_line.url);
   printf("  Version: %d\n", request->start_line.version);
   printf("\n[ Headers ]\n");
   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      if (header->value == NULL)
         break;
      printf("  %s: %s\n", header->name, header->value);
   }
   printf("\n[ Body ]\n");
   if (request->body && request->body[0] != '\0') {
      printf("%s\n", request->body);
   } else {
      printf("(empty)\n");
   }
   printf("========================================================\n\n");
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

   printf("nameSize: %zu, urlSize: %zu, versionSize: %zu\n", nameSize, urlSize, versionSize);

   if (nameSize >= HTTP_LINE_METHOD_MAX_SIZE) {
      return -1;
   }

   if (urlSize >= HTTP_LINE_URL_MAX_SIZE) {
      return -1;
   }

   if (versionSize >= HTTP_LINE_VERSION_MAX_SIZE) {
      return -1;
   }

   start_line->method = http_get_valid_method(buff_data_start, nameSize);
   printf("method: %d\n", start_line->method);
   if (start_line->method == HTTP_METHOD_INVALID) {
      return -1;
   }

   start_line->url = http_tok_alloc(request->allocator, urlStart, urlSize);
   if (!start_line->url) {
      return -1;
   }

   start_line->version = http_get_valid_version(buff_data_start + nameSize + urlSize + 2, versionSize);
   if (start_line->version == HTTP_VERSION_INVALID) {
      return -1;
   }

   size_t line_total = line_len + 2;

   request->line_end = line_total;
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
         slab_free_and_null(request->allocator, (void**)&header->name);
         header->name = NULL;
         return -1;
      }

      if (http_check_header(header->name, "Host") == 1) {
         request->host = true;
      }

      if (http_check_header(header->name, "Transfer-Encoding") == 1) {
       printf("with hosfdsfsdft\n");
         if (http_check_header(header->value, CHUNKED) == 1) {
            request->isChunked = true;
         }
      }

      if (!request->isChunked) {
         if (http_check_header(header->name, "Content-Length") == 1) {
            request->body_capacity = atoi(header->value);
            request->isContentLength = true;
         }
      }

      request->header_count = request->header_count + 1;

      ptr = line_end + 2;
   }
   
   if (!request->host || request->header_count > HTTP_MAX_HEADERS) {
      return -1;
   }

   request->header_end = request->line_end + total_size;
   http_buff_consume(buff, total_size);

   request->state = REQUEST_PARSE_BODY;
   return 0;
}

int http_request_set_ready(http_request_t *request, char *src, size_t size) {
   if (size >= HTTP_MAX_BODY_SIZE) {
      return -1;
   }
   request->body = slab_malloc(request->allocator, NULL, size);
   if (!request->body) {
      return -1;
   }
   memcpy(request->body, src, size);
   http_request_reset(request);
   request->body_capacity = size;
   request->state = REQUEST_PARSE_DONE;
   request->isReady = true; 
   return 0;
}

int http_transfer_encoding_handle(http_request_t *request, http_buffer_t *buff, size_t body_size) {
   char *buff_data_start = http_buff_data_start(buff);
   size_t sum_size = request->chunk_dirty;

   if (body_size <= sum_size) {
      return 0;
   }

   while (1) {
      char *current_start = buff_data_start + sum_size;
      char *endPoint = http_findStr(current_start, body_size - sum_size, "\r\n", 2);
      if (!endPoint) {
         break;
      }

      size_t line_len = endPoint - current_start;
      if (line_len == 0) {
         printf("Empty chunk size! Body: %s\n", buff_data_start);
         return 0;
      }

      char temp[line_len + 1];
      memcpy(temp, current_start, line_len);
      temp[line_len] = '\0';

      char *endPtr = NULL;
      size_t chunk_size = strtol(temp, &endPtr, 16);

      if (chunk_size < 0 || endPtr == temp) {
         return -1;
      }

      if (chunk_size == 0) {
         int ret = http_request_set_ready(request, buff_data_start, request->body_len);
         if (ret < 0) {
            return -1;
         }
         return 1;
      }

      size_t total_need = line_len + 2 + chunk_size + 2;
      if (body_size - sum_size < total_need) {
         return 0;
      }

      char *data_pos = current_start + line_len + 2;
      if (request->bytes_received + chunk_size > HTTP_MAX_BODY_SIZE) {
         printf("Body size exceeds limit!\n");
         return -1;
      }

      memcpy(buff_data_start + request->body_len, data_pos, chunk_size);
      request->body_len = request->body_len + chunk_size; 
      request->chunk_dirty = request->chunk_dirty + total_need;

      sum_size = sum_size + total_need;
   }

   return 0;
}

int http_content_length_handle(http_request_t *request, http_buffer_t *buff, size_t body_size) {
   size_t bytes_received = request->bytes_received - request->header_end;
   size_t body_capacity = request->body_capacity;
   char *buff_data_start = http_buff_data_start(buff);

   request->body_len = bytes_received;

   if (bytes_received >= body_capacity) {
      request->isReady = true;
      int ret = http_request_set_ready(request, buff_data_start, request->body_len);
      if (ret < 0) {
         return -1;
      }
      return 1;
   }

   return 0;
}

int http_parse_body(http_request_t *request, http_buffer_t *buff) {
   int ret = -1;
   size_t body_size = http_buff_data_len(buff);
   if (body_size >= HTTP_MAX_BODY_SIZE) {
      return ret;
   }

   if (request->isChunked) {
      return http_transfer_encoding_handle(request, buff, body_size);
   }

   if (request->isContentLength && request->body_capacity > 0) {
      return http_content_length_handle(request, buff, body_size);
   }
   return ret;
}

int http_request_parse(http_request_t *request, http_buffer_t *buff, char *data, size_t data_size) { 
   int ret = -1;

   if (request->isReady) {
      return 1;
   }
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
            http_debug_request_stats(request);
            return 1;
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

   request->bytes_received = 0;

   request->body_len = 0;
   request->body_capacity = 0;

   request->host = false;
   request->isContentLength = false;
   request->isChunked = false;
   request->chunk_dirty = 0;

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
   struct slab *allocator = request->allocator;
   http_start_line_t *start_line = &request->start_line;
   http_request_reset(request);

   slab_free_and_null(allocator, (void**)&request->body);

   slab_free_and_null(allocator, (void**)&start_line->method);
   slab_free_and_null(allocator, (void**)&start_line->url);
   slab_free_and_null(allocator, (void**)&start_line->version);

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header_t *header = &request->headers[index];
      slab_free_and_null(allocator, (void**)&header->name);
      slab_free_and_null(allocator, (void**)&header->value);
   }
}
