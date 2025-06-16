#include "http.h"
#include "pool.h"
#include "slab.h"
#include "utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>

struct slab **parser_allocators = NULL;
int g_cores_count = 0;

void *getLineType(http_message *message) {
   return message->type == HTTP_MSGTYPE_RESPONSE ? (void *)&message->start_line.status :
      message->type == HTTP_MSGTYPE_REQUEST  ? (void *)&message->start_line.request :
      NULL;
}

int find_CRLF(char *buff, size_t buff_len) {
   for (size_t index = 0; index + 1 < buff_len; index++) {
      if (buff[index] == '\r' && buff[index + 1] == '\n') {
         return index + 2;
      }
   }
   return -1;
}

int find_double_CRLF(char *buff, size_t buff_len) {
   for (size_t index = 0; index + 3 < buff_len; index++) {
      if (buff[index] == '\r' && buff[index + 1] == '\n' &&
            buff[index + 2] == '\r' && buff[index + 3] == '\n') {
         return (int)index + 4;
      }
   }
   return -1;
}

int http_content_length_parse(char *buff, size_t buff_len) {
   const char *target = "Content-Length:";
   const char *value = NULL;
   size_t target_size = strlen(target);

   for (size_t index = 0; index < buff_len - target_size; index++) {
      if (strncasecmp(&buff[index], target, target_size) == 0) {
         value = &buff[index + target_size];
         break;
      }
   }

   if (!value) {
      goto free_this_trash;
   }

   while (*value == ' ') {
      value++;
   }

   int content_length = atoi(value);
   return content_length > 0 ? content_length : 0;

free_this_trash:
   return -1;
}

void http_message_delete(char *buff, size_t *buff_len, size_t message_len) {
   size_t remain = *buff_len - message_len;
   memmove(buff, buff + message_len, remain);
   memset(buff + remain, 0, message_len);
   *buff_len = remain;
}

void http_message_extract(char *buff, size_t buff_len, size_t *message_len) {
   int endPoint = find_double_CRLF(buff, buff_len);
   if (endPoint == -1) {
      goto free_this_trash;
   }
   int content_length = http_content_length_parse(buff, (size_t)endPoint);
   *message_len = endPoint + content_length;
free_this_trash:
   return;
}

int http_find_free_message(http_parser *parser) {
   for (size_t index = 0; index < parser->messages_capacity; index++) {
      http_message message = parser->messages[index];
      if (!message.isActive) {
         return index;
      }
   }  
   return -1;
}

int http_message_init(http_parser *parser, size_t index, size_t body_size) {
   http_message *message = &parser->messages[index];
   if (!message) {
      goto free_this_trash;
   }
   message->isActive = false;
   message->isReady = false;

   message->body = slab_malloc(parser->allocator, NULL, body_size);
   if (!message->body) {
      DEBUG_FUNC("no malloc!\n");
      goto free_this_trash;
   }
   message->body_size = body_size;
   message->current_body_size = 0;

   message->isActive = true; 
free_this_trash:
   return -1;
}

void http_message_exit(http_parser *parser, size_t index) {
   http_message *message = &parser->messages[index];
   if (!message) {
      return;
   }
   if (!message->isActive) {
      return;
   }
   message->isActive = false;
   message->isReady = false;

   if (message->body) {
      slab_free(parser->allocator, message->body); 
      message->body = NULL;
   }

   message->body_size = 0;
   message->current_body_size = 0;
}

int http_get_upper_part(http_parser *parser, char *buff, size_t buff_len) {
   if (parser->type == HTTP_PARSER_LINE) {
      int start_line_end = find_CRLF(buff, buff_len);
      if (start_line_end < 0) {
         goto free_this_trash;
      }
      char tmp = buff[start_line_end - 2];
      buff[start_line_end - 2] = '\0';
      DEBUG_FUNC("start line: %s\n", buff);
      buff[start_line_end - 2] = tmp;
      parser->line_end = start_line_end;
      parser->type = HTTP_PARSER_HEADER;
   }

   if (parser->type == HTTP_PARSER_HEADER) {
      int headers_end = find_double_CRLF(buff, buff_len);
      if (headers_end < 0) {
         goto free_this_trash;
      }
      buff[headers_end - 4] = '\0';
      parser->headers_end = headers_end;
      parser->type = HTTP_PARSER_BODY;
      DEBUG_FUNC("headers: %s\n", buff + parser->line_end);
   }

   return 0;
free_this_trash:
   return -1;
}

void http_set_message_type(http_parser *parser) {
   char *buff = parser->buff;
   size_t buff_len = parser->buff_len;

   if (buff_len <= 0) {
      goto free_this_trash;
   }

   int upperPart_alive = http_get_upper_part(parser, buff, buff_len);
   if (!upperPart_alive) {
      goto free_this_trash;
   }

   int content_length = http_content_length_parse(buff + parser->line_end, parser->headers_end - parser->line_end);
   if (content_length < 0) {
      goto free_this_trash;
   }

   DEBUG_FUNC("content Length: %d\n", content_length);
free_this_trash:
   return;
}

http_consume_result http_parser_consume(http_parser *parser, const char *message, size_t len) {
   struct slab *allocator = parser->allocator;
   if (!allocator) {
      DEBUG_FUNC("No allocator!\n");
      goto free_this_trash;
   }
   if (parser->type == HTTP_PARSER_DONE) {
      parser->type = HTTP_PARSER_LINE;
   }
   http_set_message_type(parser);
   switch (parser->type) {
      case HTTP_PARSER_LINE:
         break;
      case HTTP_PARSER_HEADER:
         break;
      case HTTP_PARSER_BODY:
         break;
      default:
         break;
   }

   return HTTP_CONSUME_OK;

no_place:
   return HTTP_CONSUME_NO_ALLOC;
free_this_trash:
   return HTTP_CONSUME_PARSE_FAIL;
}

int http_parser_init(int core_count, http_parser *parser) {
   if (core_count < 0) {
      DEBUG_FUNC("you are lying[fake cores]!\n");
      goto free_this_trash;
   }

   struct slab *allocator = parser_allocators[core_count];
   if (!allocator) {
      DEBUG_FUNC("No allocator!\n");
      goto free_this_trash;
   }

   parser->content_length = 0;
   parser->buff_len = 0;
   parser->content_received = 0;
   parser->type = HTTP_PARSER_DONE;
   parser->allocator = allocator;
   memset(parser->buff, 0, sizeof(parser->buff));

   return 0;

free_this_trash:
   return -1;
}

int http_parser_allocator_init(int cores_count) {
   g_cores_count = cores_count;
   parser_allocators = malloc(sizeof(struct slab *) * g_cores_count);
   if (!parser_allocators) {
      DEBUG_FUNC("no allocators\n");
      goto free_this_trash;
   }

   for (int index = 0; index < g_cores_count; index++) {
      parser_allocators[index] = slab_init(1024 * 1024);
      if (!parser_allocators[index]) {
         DEBUG_FUNC("No allocator index: %d\n", index);
         for (int new = 0; new < index; new++) {
            slab_exit(parser_allocators[new]);
         }
         parser_allocators = NULL;
         goto free_everything;
      }
   }

   return 0;

free_everything:
   http_parser_allocator_exit();
free_this_trash:
   return -1;
}

void http_parser_allocator_exit() {
   if (parser_allocators) {
      for (int index = 0; index < g_cores_count; index++) {
         if (parser_allocators[index]) {
            slab_exit(parser_allocators[index]);
         }
      }
   }
   free(parser_allocators);
   parser_allocators = NULL;
   g_cores_count = 0;
}
