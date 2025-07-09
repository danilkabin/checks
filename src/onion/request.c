/*#include "request.h"
#include "http.h"

#include "onion.h"
#include "slab.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNKED "chunked"

typedef enum {
   REQUEST_LINE = 1,
   ERROR_REQUEST_LINE = -1,
   MORE_REQUEST_LINE = 0,
   REQUEST_HEADER = 1,
   ERROR_REQUEST_HEADER = -1,
   MORE_REQUEST_HEADER = 0,
   REQUEST_BODY = 1,
   ERROR_REQUEST_BODY = -1,
   MORE_REQUEST_BODY = 0 
} onion_token_report;

char *onion_http_tok_alloc(struct onion_slab *allocator, char *src, size_t len) {
   char *ret = onion_slab_malloc(allocator, NULL, len + 1);
   if (!ret) {
      return NULL;
   }
   memcpy(ret, src, len);
   ret[len] = '\0';
   return ret;
}

int onion_http_request_append(onion_http_request_t *request, size_t data_size) {
   request->bytes_received = request->bytes_received + data_size;
   return 0;
}

void onion_http_cleanup_headers(onion_http_request_t *request, int up_to_index) {
   for (int i = 0; i < up_to_index; i++) {
      onion_slab_free_and_null(request->allocator, (void**)&request->headers[i].name);
      onion_slab_free_and_null(request->allocator, (void**)&request->headers[i].value);
   }
}

int onion_http_check_header(const char *name, const char *target) {
   return strcasecmp(name, target) == 0 ? 1 : -1;
}

void onion_http_debug_request_stats(onion_http_request_t *request) {
   printf("\n====================[ HTTP REQUEST ]====================\n");
   printf("[ Start Line ]\n");
   printf("  Method : %d\n", request->start_line.method);
   printf("  URL    : %s\n", request->start_line.url);
   printf("  Version: %d\n", request->start_line.version);
   printf("\n[ Headers ]\n");
   for (int index = 0; index < ONION_HTTP_MAX_HEADERS; index++) {
      ontion_http_header_t *header = &request->headers[index];
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

int http_is_complete() {
   return 0;
}

int onion_http_parse_start_line(onion_http_request_t *request, onion_http_buffer_t *buff) {
   http_start_line_t *start_line = &request->start_line;

   char *buff_data_start = onion_http_buff_data_start(buff);
   size_t buff_data_len = onion_http_buff_data_len(buff);

   if (buff_data_len >= ONION_HTTP_LINE_MAX_SIZE) {
      goto unsuccessfull;
   }

   char *endVersion = onion_stringa(buff_data_start, buff_data_len, "\r\n", 2);
   if (!endVersion) {
      return MORE_REQUEST_LINE;
   }

   size_t line_len = endVersion - buff_data_start;

   if (line_len <= 0) {
      goto unsuccessfull;
   }

   char *endName = onion_chacha(buff_data_start, line_len, ' ');
   if (!endName) {
      goto unsuccessfull;
   }

   char *urlStart = endName + 1;
   char *endUrl = onion_chacha(urlStart, line_len - (urlStart - buff_data_start), ' ');
   if (!endUrl) {
      goto unsuccessfull;
   }

   size_t nameSize = endName - buff_data_start;
   size_t urlSize = endUrl - urlStart;

   char *versionStart = endUrl + 1;
   size_t versionSize = endVersion - versionStart;

   printf("nameSize: %zu, urlSize: %zu, versionSize: %zu\n", nameSize, urlSize, versionSize);

   if (nameSize >= ONION_HTTP_LINE_METHOD_MAX_SIZE || nameSize == 0) {
      goto unsuccessfull;
   }

   if (urlSize >= ONION_HTTP_LINE_URL_MAX_SIZE || urlSize == 0) {
      goto unsuccessfull;
   }

   if (versionSize >= ONION_HTTP_LINE_VERSION_MAX_SIZE || versionSize == 0) {
      goto unsuccessfull;
   }

   if (onion_http_check_in_range(urlStart, urlSize, 32, 126, NULL, 0) < 0) {
      goto unsuccessfull;
   }

   start_line->method = onion_http_get_valid_method(buff_data_start, nameSize);
   printf("method: %d\n", start_line->method);
   if (start_line->method == ONION_HTTP_METHOD_INVALID) {
      return -1;
   }

   start_line->url = onion_http_tok_alloc(request->allocator, urlStart, urlSize);
   if (!start_line->url) {
      goto unsuccessfull;
   }

   start_line->version = onion_http_get_valid_version(buff_data_start + nameSize + urlSize + 2, versionSize);
   if (start_line->version == ONION_HTTP_VERSION_INVALID) {
      goto unsuccessfull;
   }

   size_t line_total = line_len + 2;

   request->line_end = line_total;
   onion_http_buff_consume(buff, line_total);

   request->state = REQUEST_PARSE_HEADERS;
   return REQUEST_LINE;
unsuccessfull:
   return ERROR_REQUEST_LINE;
}

int onion_http_parse_headers(onion_http_request_t *request, onion_http_buffer_t *buff) {
   char *buff_data_start = onion_http_buff_data_start(buff);
   size_t buff_data_len = onion_http_buff_data_len(buff);

   char *endPoint = onion_stringa(buff_data_start, buff_data_len, "\r\n\r\n", 4);
   if (!endPoint) {
      if (buff_data_len >= ONION_HTTP_MAX_HEADER_SIZE) {
         goto unsuccessfull;
      }
      return MORE_REQUEST_HEADER;
   }

   char *ptr = buff_data_start;
   size_t total = buff_data_len;
   size_t total_size = endPoint - buff_data_start + 4;

   for (int index = 0; index < ONION_HTTP_MAX_HEADERS + 1; index++) {
      ontion_http_header_t *header = &request->headers[index];
      char *line_end = onion_stringa(ptr, total - (ptr - buff_data_start), "\r\n", 2);
      if (!line_end) {
         break;
      }
      if (line_end == ptr) {
         break;
      }
      size_t header_len = line_end - ptr;

      char *colon = onion_stringa(ptr, header_len, ":", 1);
      if (!colon) {
         goto unsuccessfull;
      }

      size_t name_len = colon - ptr;
      if (name_len >= ONION_HTTP_HEADER_NAME_SIZE) {
         printf("Big size\n");
         goto unsuccessfull;
      }

      if (onion_http_get_valid_header_name(ptr, name_len) < 0) {
         goto unsuccessfull;
      } 

      header->name = onion_http_tok_alloc(request->allocator, ptr, name_len);
      if (!header->name) {
         printf("No malloc!\n");
         goto unsuccessfull;
      }

      char *val_start = colon + 1;
      while (*val_start == ' ') {
         val_start++;
      }
      size_t val_len = line_end - val_start;

      if (val_len >= ONION_HTTP_HEADER_VALUE_SIZE) {
         goto unsuccessfull;
      }

      unsigned char exceptions[] = {9};
      if (onion_http_check_in_range(val_start, val_len, 32, 255, exceptions, sizeof(exceptions)) < 0) {
         goto unsuccessfull;
      }

      header->value = onion_http_tok_alloc(request->allocator, val_start, line_end - val_start);
      if (!header->value) {
         printf("No malloc!\n");
         onion_http_cleanup_headers(request, index);
         goto unsuccessfull;
      }

      if (onion_http_check_header(header->name, "Host") == 1) {
         request->host = true;
      }

      if (onion_http_check_header(header->name, "Transfer-Encoding") == 1) {
         if (onion_http_check_header(header->value, CHUNKED) == 1) {
            request->isChunked = true;
         }
      }

      if (!request->isChunked) {
         if (onion_http_check_header(header->name, "Content-Length") == 1) {
            request->body_capacity = atoi(header->value);
            request->isContentLength = true;
         }
      }

      request->header_count = request->header_count + 1;

      ptr = line_end + 2;
   }

   if (!request->host || request->header_count > ONION_HTTP_MAX_HEADERS) {
      goto unsuccessfull;
   }

   request->header_end = request->line_end + total_size;
   onion_http_buff_consume(buff, total_size);

   request->state = REQUEST_PARSE_BODY;
   return REQUEST_HEADER;
unsuccessfull:
   return ERROR_REQUEST_HEADER;
}

int http_request_set_ready(onion_http_request_t *request, char *src, size_t size) {
   if (size >= ONION_HTTP_MAX_BODY_SIZE) {
      return -1;
   }
   request->body = onion_slab_malloc(request->allocator, NULL, size);
   if (!request->body) {
      return -1;
   }
   memcpy(request->body, src, size);
   onion_http_request_reset(request);
   request->body_capacity = size;
   request->state = REQUEST_PARSE_DONE;
   request->isReady = true; 
   return 0;
}

int onion_http_transfer_encoding_handle(onion_http_request_t *request, onion_http_buffer_t *buff, size_t body_size) {
   char *buff_data_start = onion_http_buff_data_start(buff);
   size_t sum_size = request->chunk_dirty;

   if (body_size <= sum_size) {
      return MORE_REQUEST_BODY;
   }

   while (1) {
      char *current_start = buff_data_start + sum_size;
      char *endPoint = onion_stringa(current_start, body_size - sum_size, "\r\n", 2);
      if (!endPoint) {
         break;
      }

      size_t line_len = endPoint - current_start;
      if (line_len == 0 || line_len > 16) {
         printf("Empty chunk size! Body: %s\n", buff_data_start);
         return MORE_REQUEST_BODY;
      }

      char temp[line_len + 1];
      memcpy(temp, current_start, line_len);
      temp[line_len] = '\0';

      char *endPtr = NULL;
      size_t chunk_size = strtol(temp, &endPtr, 16);
      if (chunk_size < 0 || endPtr == temp || chunk_size >= ONION_HTTP_MAX_BODY_SIZE) {
         return ERROR_REQUEST_BODY;
      }

      if (chunk_size == 0) {
         int ret = http_request_set_ready(request, buff_data_start, request->body_len);
         if (ret < 0) {
            return ERROR_REQUEST_BODY;
         }
         return REQUEST_BODY;
      }

      size_t total_need = line_len + 2 + chunk_size + 2;
      if (body_size - sum_size < total_need) {
         return MORE_REQUEST_BODY;
      }

      char *data_pos = current_start + line_len + 2;
      if (request->bytes_received + chunk_size >= ONION_HTTP_MAX_BODY_SIZE) {
         printf("Body size exceeds limit!\n");
         return ERROR_REQUEST_BODY;
      }

      memcpy(buff_data_start + request->body_len, data_pos, chunk_size);
      request->body_len = request->body_len + chunk_size; 
      request->chunk_dirty = request->chunk_dirty + total_need;

      sum_size = sum_size + total_need;
   }

   return MORE_REQUEST_BODY;
}

int onion_http_content_length_handle(onion_http_request_t *request, onion_http_buffer_t *buff, size_t body_size) {
   size_t bytes_received = request->bytes_received - request->header_end;
   size_t body_capacity = request->body_capacity;
   char *buff_data_start = onion_http_buff_data_start(buff);

   request->body_len = bytes_received;

   if (bytes_received >= body_capacity) {
      request->isReady = true;
      int ret = http_request_set_ready(request, buff_data_start, request->body_len);
      if (ret < 0) {
         return ERROR_REQUEST_BODY;
      }
      return REQUEST_BODY;
   }

   return MORE_REQUEST_BODY;
}

int onion_http_parse_body(onion_http_request_t *request, onion_http_buffer_t *buff) {
   int ret = -1;
   size_t body_size = onion_http_buff_data_len(buff);
   if (body_size >= ONION_HTTP_MAX_BODY_SIZE) {
      return ret;
   }
   
   if (request->isChunked) {
      return onion_http_transfer_encoding_handle(request, buff, body_size);
   }

   if (request->isContentLength && request->body_capacity > 0) {
      return onion_http_content_length_handle(request, buff, body_size);
   }
   return ret;
}

int onion_http_request_parse(onion_http_request_t *request, onion_http_buffer_t *buff, char *data, size_t data_size) {
   int ret = 0;

   if (request->isReady) {
      return 1;
   }
   while (1) {
      switch (request->state) {
         case REQUEST_PARSE_START_LINE:
            ret = onion_http_parse_start_line(request, buff);
            if (ret != REQUEST_LINE) {
               return ret;
            }
            break;
         case REQUEST_PARSE_HEADERS:
            ret = onion_http_parse_headers(request, buff);
            if (ret != REQUEST_HEADER) {
               return ret;
            }
            break;
         case REQUEST_PARSE_BODY:
            ret = onion_http_parse_body(request, buff);
            if (ret != REQUEST_BODY) {
               return ret;
            }

            onion_http_debug_request_stats(request);
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

void onion_http_request_reset(onion_http_request_t *request) {
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

int onion_http_request_init(onion_http_request_t *request, struct onion_slab *allocator) {
   onion_http_request_reset(request);

   request->allocator = allocator;

   request->isActive = true;
   return 0;
}

int onion_http_request_reinit(onion_http_request_t *request, struct onion_slab *allocator) {
   onion_http_request_exit(request);
   return onion_http_request_init(request, allocator);
}

void onion_http_request_exit(onion_http_request_t *request) {
   struct onion_slab *allocator = request->allocator;
   http_start_line_t *start_line = &request->start_line;
   onion_http_request_reset(request);

   onion_slab_free_and_null(allocator, (void**)&request->body);
   onion_slab_free_and_null(allocator, (void**)&start_line->url);

   for (int index = 0; index < ONION_HTTP_MAX_HEADERS; index++) {
      ontion_http_header_t *header = &request->headers[index];
      onion_slab_free_and_null(allocator, (void**)&header->name);
      onion_slab_free_and_null(allocator, (void**)&header->value);
   }
}*/
