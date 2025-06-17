#include "http.h"
#include "pool.h"
#include "slab.h"
#include "utils.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>

struct slab **parser_allocators = NULL;
int g_cores_count = 0;

const char *http_method_m[] = {
#define X(name) #name,
   HTTP_METHOD_LIST
#undef X
      "HTTP_METHOD_UNKNOWN"
};

const char *http_version_m[] = {
#define V(name, str) str,
   HTTP_VERSION_LIST
#undef V
      "HTTP_VERSION_UNKNOWN"
};

int find_CRLF(char *buff, size_t buff_len) {
   for (size_t index = 0; index + 1 < buff_len; index++) {
      if (buff[index] == '\r' && buff[index + 1] == '\n') {
         return index + 2;
      }
   }
   return -1;
}

int find_double_CRLF(char *buff, size_t buff_len) {
   if (buff_len < 4) {
      return -1;
   }
   for (size_t index = 0; index + 3 < buff_len; index++) {
      if (buff[index] == '\r' && buff[index + 1] == '\n' &&
            buff[index + 2] == '\r' && buff[index + 3] == '\n') {
         return (int)(index + 4);
      }
   }
   return -1;
}

http_method http_parse_method(const char *method) {
   for (int index = 0; index < HTTP_METHOD_UNKNOWN; index++) {
      if (strcmp(method, http_method_m[index]) == 0) {
         DEBUG_FUNC("method: %s\n", http_method_m[index]);
         return (http_method)index;
      } 
   }
   return HTTP_METHOD_UNKNOWN;
}

http_version http_parse_version(const char *version) {
   char clean[16] = {0};
   size_t len = strcspn(version, "\r\n");
   if (len >= sizeof(clean)) {
      len = sizeof(clean) - 1;
   }
   strncpy(clean, version, len);
   clean[len] = '\0';

   for (int index = 0; index < HTTP_VERSION_UNKNOWN; index++) {
      if (strcmp(clean, http_version_m[index]) == 0) {
         DEBUG_FUNC("version: %s\n", http_version_m[index]);
         return (http_version)index;
      }
   }
   return HTTP_VERSION_UNKNOWN;
}

int http_string_parse(char *buff, size_t buff_len, const char *target) {
   const char *value = NULL;
   size_t target_size = strlen(target);

   if (buff_len < target_size) {
      return -1;
   }

   for (size_t index = 0; index < buff_len - target_size; index++) {
      if (strncasecmp(&buff[index], target, target_size) == 0) {
         value = &buff[index + target_size];
         break;
      }
   }

   if (!value) {
      return -1;
   }

   while (*value == ' ') {
      value++;
   }

   char *endptr;
   long result = strtol(value, &endptr, 10);
   if (value == endptr || result < 0) {
      return -1;
   }
   return (int)result;
}

void clear_buffer(http_parser *parser) {
   memset(parser->buff, 0, sizeof(parser->buff));
   parser->buff_len = 0;

   parser->line_end = 0;
   parser->headers_end = 0;
   parser->body_end = 0;

   parser->current_message_index = -1;
   parser->bytes_received = 0;
   parser->type = HTTP_PARSER_DONE;
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

int http_message_init(http_parser *parser, size_t index, http_message_type type, size_t body_size) {
   if (!type || type == HTTP_MSGTYPE_NULL) {
      goto free_this_trash;
   }
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
   message->type = type;

   message->isActive = true;

   parser->body_end = parser->headers_end + message->body_size;
   return 0;
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

      char tmp = buff[headers_end - 4];
      buff[headers_end - 4] = '\0';
      DEBUG_FUNC("headers: %s\n", buff + parser->line_end);
      buff[headers_end - 4] = tmp;

      parser->headers_end = headers_end;
      parser->type = HTTP_PARSER_BODY;
   }

   return 0;
free_this_trash:
   return -1;
}

int http_validate_start_line(http_parser *parser, http_message *message) {
   char temp[parser->buff_len];
   strncpy(temp, parser->buff, parser->buff_len);
   temp[parser->buff_len] = '\0';

   http_method method = http_parse_method(strtok(temp, " "));
   char *path = strtok(NULL, " ");
   http_version version = http_parse_version(strtok(NULL, " "));

   if (!method || !path || !version) {
      DEBUG_FUNC("No string\n");
      goto free_this_trash;
   } 

   if (method == HTTP_METHOD_UNKNOWN) {
      DEBUG_FUNC("Unknown method!\n");
      goto invald_method;
   }

   if (version == HTTP_VERSION_UNKNOWN) {
      DEBUG_FUNC("Unknown version!\n");
      goto invalid_version;
   }

   if (message) {
      request_line request = message->start_line.request;
      request.method = method;
      request.target = path;
      request.version = version;
   }

   DEBUG_FUNC("method: %d path: %s version: %d\n", method, path, version);
   return 0;
invald_method:
   return -3;
invalid_version:
   return -2;
free_this_trash:
   return -1;
}

int http_validate_headers(http_parser *parser, http_message *message) {
   char temp[parser->buff_len];
   strncpy(temp, parser->buff, parser->buff_len);
   temp[parser->buff_len] = '\0';

   http_method method = http_parse_method(strtok(temp, " "));
   char *path = strtok(NULL, " ");
   http_version version = http_parse_version(strtok(NULL, " "));

   if (!method || !path || !version) {
      DEBUG_FUNC("No string\n");
      goto free_this_trash;
   }

   if (method == HTTP_METHOD_UNKNOWN) {
      DEBUG_FUNC("Unknown method!\n");
      goto invald_method;
   }

   if (version == HTTP_VERSION_UNKNOWN) {
      DEBUG_FUNC("Unknown version!\n");
      goto invalid_version;
   }

   if (message) {
      request_line request = message->start_line.request;
      request.method = method;
      request.target = path;
      request.version = version;
   }

   DEBUG_FUNC("method: %d path: %s version: %d\n", method, path, version);
   return 0;
invald_method:
   return -3;
invalid_version:
   return -2;
free_this_trash:
   return -1;
}

int http_start_line_parse(http_parser *parser, http_message *message) {
   int ret = -1;
   if (message->type == HTTP_MSGTYPE_NULL) {
      goto free_this_trash;
   }

   if (message->type == HTTP_MSGTYPE_REQUEST) {
      ret = http_validate_start_line(parser, message);
   } else if (message->type == HTTP_MSGTYPE_RESPONSE) {

   }

   if (ret < 0) {
      goto invalid_start_line;
   }

   return 0;
invalid_start_line:
   return -2;
free_this_trash:
   return -1;
}

int http_validate_header(char *line, size_t line_len, http_header *header) {
   char *colon = strchr(line, ':');
   if (!colon) {
      return - 1;
   }

   size_t name_len = colon - line;
   size_t value_len = line_len - name_len; 
   for (int index = 0; index < (int)name_len; index++) {
      if (isspace((unsigned char)line[index])) {
         return -1; 
      }
   }

   char *value = colon + 1;
   while (*value && isspace((unsigned char)*value)) {
      value++;
   }
   if (*value == '\0') {
      return -1;
   }

   char write_name[HTTP_HEADER_NAME_SIZE];
   char write_value[HTTP_HEADER_VALUE_SIZE];

   strncpy(write_name, line, name_len);
   write_name[name_len] = '\0';

   strncpy(write_value, value, sizeof(*write_value) - 1);
   write_value[sizeof(write_value) - 1] = '\0';

   if (header) {
      strncpy(header->name, write_name, HTTP_HEADER_NAME_SIZE);
      header->name[HTTP_HEADER_NAME_SIZE - 1] = '\0';
      strncpy(header->value, write_value, HTTP_HEADER_VALUE_SIZE);
      header->value[HTTP_HEADER_VALUE_SIZE - 1] = '\0';
   }

   return 0;
}

int http_headers_parse(http_parser *parser, http_message *message, const char *headers) {
   int ret;

   const char *line_start = headers;
   const char *header_end = strstr(headers, "\r\n\r\n");
   size_t header_size = header_end - line_start + 4;
   size_t current_size = 0;
   size_t count = 0;

   if (!header_end) {
      return -1;
   }

   while (1) {
      if (count > HTTP_MAX_HEADERS) {
         return -1;
      } 

      const char *line_end = strstr(line_start, "\r\n");
      if (!line_end || (current_size + 2 >= header_size)) {
         break;
      }

      size_t line_len = line_end - line_start;
      char line[HTTP_HEADER_VALUE_SIZE];
      if (line_len > sizeof(line)) {
         return -1;
      }

      strncpy(line, line_start, line_len);
      line[line_len] = '\0';

      http_header *header = &message->headers[count];
      if (!header) {
         return -1;
      }

      if (!header->name && !header->value) {
         header->name = slab_malloc(parser->allocator, NULL, HTTP_HEADER_NAME_SIZE);
         if (!header->name) {
            return -1;
         }
         header->value = slab_malloc(parser->allocator, NULL, HTTP_HEADER_VALUE_SIZE);
         if (!header->value) {
            return -1;
         }
      }

      int valid = http_validate_header(line, line_len, &message->headers[count]);
      if (valid < 0) {
         return -1;
      }

      current_size = current_size + line_len + 2;
      count = count + 1;
      line_start = line_end + 2;

      DEBUG_FUNC("heeaders: %zu, line_len; %zu, current_size:%zu header size: %zu \n", count, line_len, current_size, header_size);
   }

   return 0;
}

void http_set_message_type(http_parser *parser, size_t bytes_received) {
   int ret;
   char *buff = parser->buff;
   size_t buff_len = parser->buff_len;

   if (buff_len <= 0) {
      return;
   }

   parser->bytes_received = parser->bytes_received + bytes_received;

   int upperPart_alive = http_get_upper_part(parser, buff, buff_len);
   if (upperPart_alive < 0) {
      return;
   }

   int content_length = http_string_parse(buff + parser->line_end, parser->headers_end - parser->line_end, "Content-Length:");
   if (content_length < 0) {
      return;
   }

   int current_message_index = parser->current_message_index; 
   bool needInit = false;
   if (current_message_index < 0) {
      current_message_index = http_find_free_message(parser);
      if (current_message_index < 0) {
         return;
      }
      needInit = true;
   }
   parser->current_message_index = current_message_index;

   DEBUG_FUNC("current_message_index: %d\n", current_message_index);

   http_message *message = &parser->messages[current_message_index];
   if (!message) {
      return;
   }

   if (needInit) {
      ret = http_message_init(parser, current_message_index, HTTP_MSGTYPE_REQUEST, content_length);
      if (ret < 0) {
         memset(message, 0, sizeof(http_message));
         return;
      }
   }

   ret = http_start_line_parse(parser, message);
   if (ret < 0) {
      clear_buffer(parser);
      return;
   }

   ret = http_headers_parse(parser, message, parser->buff + parser->line_end);
   if (ret < 0) {
      clear_buffer(parser);
      return;
   }

   size_t body_bytes_in_buff = parser->bytes_received - parser->headers_end;
   if (body_bytes_in_buff > message->current_body_size) {
      size_t new_bytes = body_bytes_in_buff - message->current_body_size;
      size_t available_space = message->body_size - message->current_body_size;
      if (new_bytes > available_space) {
         new_bytes = available_space;
      }

      if (new_bytes > 0) {
         memcpy(message->body + message->current_body_size, parser->buff + parser->headers_end + message->current_body_size, new_bytes);
         message->current_body_size = message->current_body_size + new_bytes;
      }
   }

   if (message->current_body_size >= message->body_size) {
      message->isReady = true;
      clear_buffer(parser);
   }

   for (size_t index = 0; index < parser->messages_capacity; index++) {
      http_message *temp_msg = &parser->messages[index];
      DEBUG_FUNC("index: %d msg: %s\n", (int)index, temp_msg->body);
   }
}

http_consume_result http_parser_consume(http_parser *parser, const char *message, size_t bytes_received) {
   struct slab *allocator = parser->allocator;
   if (!allocator) {
      DEBUG_FUNC("No allocator!\n");
      goto free_this_trash;
   }
   if (parser->type == HTTP_PARSER_DONE) {
      parser->type = HTTP_PARSER_LINE;
   }
   http_set_message_type(parser, bytes_received);
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

   parser->current_message_index = -1;
   parser->buff_len = 0;
   parser->messages_capacity = HTTP_MAX_MESSAGES;
   parser->type = HTTP_PARSER_DONE;
   parser->allocator = allocator;
   memset(parser->buff, 0, sizeof(parser->buff));

   for (size_t index = 0; index < parser->messages_capacity; index++) {
      http_message *message = &parser->messages[index];
      message->isActive = false;
   }

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
