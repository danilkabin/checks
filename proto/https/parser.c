#include "http.h"
#include "parser.h"
#include "request.h"
#include "slab.h"

#include <stdio.h>
#include <string.h>

int http_find_free_request(http_parser_t *parser) {
   int ret = -1;

   for (int index = 0; index < HTTP_MAX_REQUESTS; index++) {
      http_request_t *request = &parser->requests[parser->req_pos];

      if (!request->isReady) {
         return parser->req_pos;
      }

      parser->req_pos = (parser->req_pos + 1) %HTTP_MAX_REQUESTS;
   }

   return ret;
}

int http_parser_request_append(http_parser_t *parser, http_request_t *request, char *data, size_t data_size) {
   if (request->state == REQUEST_PARSE_ERROR) {
      return -1;
   }

   http_buffer_t *buff = &parser->req_buff;

   int ret = http_buff_append(buff, data, data_size);
   if (ret < 0) {
      return ret;
   }

   http_request_append(request, data_size);

   return 0;
}

http_parser_request_type http_parser_request(http_parser_t *parser, char *data, size_t size) {
   int index = http_find_free_request(parser);
   if (index < 0) {
      return HTTP_PARSER_REQUEST_LIMIT;
   }

   int ret;
   http_request_t *request = &parser->requests[index];

   if (!request->isActive) {
      ret = http_request_init(request, parser->req_msg_allocator); 
      if (ret < 0) {
         goto parser_error;
      }
   }

   ret = http_parser_request_append(parser, request, data, size);
   if (ret < 0) {
      printf("http_parser_request_append failed\n");
      goto parser_error_exit;
   }

   ret = http_request_parse(request, &parser->req_buff, data, size);
   if (ret < 0) {
      printf("http_request_parse failed\n");
      goto parser_error_exit;
   }

   return HTTP_PARSER_REQUEST_OK;
parser_error_exit:
   http_request_exit(request);
parser_error:
   return HTTP_PARSER_REQUEST_ERROR;
}

int http_parser_init(http_parser_t *parser, struct slab *allocator, struct slab *msg_allocator) {
   int ret;

   parser->req_allocator = allocator;
   parser->req_msg_allocator = msg_allocator;

   printf("HTTP_MAX_MESSAGE_SIZE: %d, HTTP_MAX_HEADER_SIZE %d, HTTP_MAX_BODY_SIZE %d, HTTP_LINE_MAX_SIZE %d\n", HTTP_MAX_MESSAGE_SIZE, HTTP_MAX_HEADER_SIZE, HTTP_MAX_BODY_SIZE, HTTP_LINE_MAX_SIZE);

   ret = http_buff_init(parser->req_allocator, &parser->req_buff, HTTP_LINE_MAX_SIZE, HTTP_MAX_MESSAGE_SIZE);
   if (ret < 0) {
      printf("http_buffer_t initialization failed.\n");
      goto unsuccessfull;
   }

   parser->req_active = 0;
   parser->req_pos = 0;
   parser->req_head = 0;

   return 0;
unsuccessfull:
   return -1;
}

void http_parser_exit(http_parser_t *parser) {
   for (int index = 0; index < parser->req_active; index++) {
      http_request_t *request = &parser->requests[index];
      http_request_exit(request);
   }

   if (parser->req_buff.data) {
      http_buff_exit(&parser->req_buff);
   }

   if (parser->req_msg_allocator) {
      parser->req_msg_allocator = NULL;
   }

   if (parser->req_allocator) {
      parser->req_allocator = NULL;
   }
}
