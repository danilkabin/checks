/*#include "http.h"
#include "parser.h"
#include "request.h"
#include "slab.h"

#include <stdio.h>
#include <string.h>

int onion_http_find_free_request(onion_http_parser_t *parser) {
   int ret = -1;

   for (int index = 0; index < ONION_HTTP_MAX_REQUESTS; index++) {
      int pos = (parser->req_pos + index) % ONION_HTTP_MAX_REQUESTS;
      onion_http_request_t *request = &parser->requests[pos];
      if (!request->isReady) {
         return pos;
      }
   }

   return ret;
}

int onion_http_parser_request_append(onion_http_parser_t *parser, onion_http_request_t *request, char *data, size_t data_size) {
   if (request->state == REQUEST_PARSE_ERROR) {
      return -1;
   }

   onion_http_buffer_t *buff = &parser->req_buff;

   int ret = onion_http_buff_append(buff, data, data_size);
   if (ret < 0) {
      return ret;
   }

   onion_http_request_append(request, data_size);

   return 0;
}

int onion_http_parser_request(onion_http_parser_t *parser, char *data, size_t size) {
   int index = onion_http_find_free_request(parser);
   if (index < 0) {
      return ONION_HTTP_PARSER_REQUEST_LIMIT;
   }

   int ret;
   onion_http_request_t *request = &parser->requests[index];

   if (!request->isActive) {
      ret = onion_http_request_init(request, parser->req_msg_allocator);
      if (ret < 0) {
         goto parser_error;
      }
   }

   ret = onion_http_parser_request_append(parser, request, data, size);
   if (ret < 0) {
      printf("onion_http_parser_request_append failed\n");
      goto parser_error_exit;
   }

   ret = onion_http_request_parse(request, &parser->req_buff, data, size);
   if (ret < 0) {
      printf("http_request_parse failed\n");
      goto parser_error_exit;
   }

   if (ret > 0) {
      printf("INDEX request confirmed: %d\n", index);
      ret = onion_http_buff_reinit(parser->req_allocator, &parser->req_buff, ONION_HTTP_LINE_MAX_SIZE, ONION_HTTP_MAX_MESSAGE_SIZE);
      if (ret < 0) {
         goto parser_error_exit;
      }
   }

   return ONION_HTTP_PARSER_REQUEST_OK;
   parser_error_exit:
   onion_http_buff_exit(&parser->req_buff);
   onion_http_request_exit(request);
   parser_error:
   return ONION_HTTP_PARSER_REQUEST_ERROR;
}

int onion_http_parser_init(onion_http_parser_t *parser, struct onion_slab *allocator, struct onion_slab *msg_allocator) {
   int ret;
   parser->req_allocator = allocator;
   parser->req_msg_allocator = msg_allocator;

   ret = onion_http_buff_init(parser->req_allocator, &parser->req_buff, ONION_HTTP_LINE_MAX_SIZE, ONION_HTTP_MAX_MESSAGE_SIZE);
   if (ret < 0) {
      printf("onion_http_buffer_t initialization failed.\n");
      goto unsuccessfull;
   }

   parser->req_active = 0;
   parser->req_pos = 0;
   parser->req_head = 0;

   return 0;
   unsuccessfull:
   return -1;
}

void onion_http_parser_exit(onion_http_parser_t *parser) {
   for (int index = 0; index < parser->req_active; index++) {
      onion_http_request_t *request = &parser->requests[index];
      onion_http_request_exit(request);
   }

   if (parser->req_buff.data) {
      onion_http_buff_exit(&parser->req_buff);
   }

   if (parser->req_msg_allocator) {
      parser->req_msg_allocator = NULL;
   }

   if (parser->req_allocator) {
      parser->req_allocator = NULL;
   }
}*/
