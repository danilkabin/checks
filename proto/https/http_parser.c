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

#define CHUNKED "chunked"

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

int findstr(char *str, size_t str_size, char *find, size_t find_size) {
   if (!str || str_size <= 0 || !find || find_size <= 0) {
      return -1;
   }

   for (size_t index = 0; index <= str_size - find_size; index++) {
      if (memcmp(&str[index], find, find_size) == 0) {
         return (int)index;
      }
   }

   return -1;
}

int http_check_on_host(char *headers, size_t headers_size) {
   size_t offset = 0;

   while (offset < headers_size) {
      char *line_start = headers + offset;
      int line_len = findstr(line_start, headers_size, "\r\n", 2);
      if (line_len < 0) {
         return -1;
      }

      if (line_len > 5) {
         if (strncasecmp(line_start, "Host", 4) == 0 && (line_start[4] == ':')) {
            return 0;
         }
      }

      offset += line_len + 2;
   }
   return -1;
}

http_method http_parse_method(const char *method) {
   for (int index = 0; index < HTTP_METHOD_UNKNOWN; index++) {
      if (strcmp(method, http_method_m[index]) == 0) {
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
         return (http_version)index;
      }
   }
   return HTTP_VERSION_UNKNOWN;
}

int http_string_parse(char *buff, size_t buff_len, const char *target, char *temp, size_t temp_size) {
   char *value = NULL;
   size_t target_size = strlen(target);

   if (!temp || temp_size <= 0) {
      return -1;
   }

   if (buff_len < target_size) {
      return -1;
   }

   for (size_t index = 0; index <= buff_len - target_size; index++) {
      if (strncasecmp(&buff[index], target, target_size) == 0) {
         value = &buff[index + target_size];
         break;
      }
   }

   if (!value) {
      return -1;
   }

   while (*value == ' ' || *value == '\t') {
      value++;
   }

   size_t remaining_len = buff_len - (value - buff);
   int line_end = findstr(value, remaining_len, "\r\n", 2);
   if (line_end < 0) {
      return -1;
   }

   size_t value_len = (size_t)line_end;
   if (value_len >= temp_size) {
      return -1;
   }

   memcpy(temp, value, value_len);
   temp[value_len] = '\0';

   return atoi(temp);
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
   message->isUpperParsed = false;
   message->isChunked = parser->isChunked ? true : false;

   message->body = slab_malloc(parser->allocator, NULL, body_size);
   message->body_size = 0;

   if (!message->body) {
      DEBUG_FUNC("no malloc!\n");
      goto free_this_trash;
   }

   message->body_size = body_size;
   message->current_body_size = 0;
   message->type = type;
   DEBUG_FUNC("body size: %zu\n", body_size);
   if (message->type == HTTP_MSGTYPE_REQUEST) {
      message->request.method = HTTP_METHOD_UNKNOWN;
      message->request.target = NULL;
      message->request.version = HTTP_VERSION_UNKNOWN;
   } else if (message->type == HTTP_MSGTYPE_RESPONSE) {
      message->response.code = 0;
      message->response.body = NULL;
      message->response.headers = NULL;
   }

   parser->body_end = parser->headers_end + message->body_size;
   message->isActive = true;
   return 0;
free_this_trash:
   return -1;
}

void http_message_exit(http_parser *parser, size_t index) {
   http_message *message = &parser->messages[index];
   if (!message) {
      return;
   }

   if (message->isActive == false) {
      return;
   }
   message->type = HTTP_MSGTYPE_NULL;

   message->isActive = false;
   message->isReady = false;
   message->isUpperParsed = false;

   if (message->body) {
      DEBUG_FUNC("BODY DELETED!\n");
      slab_free(parser->allocator, message->body); 
      message->body = NULL;
   }

   for (int index = 0; index < HTTP_MAX_HEADERS; index++) {
      http_header *header = &message->headers[index];
      if (header->name) {
         slab_free(parser->allocator, header->name);
         header->name = NULL;
      }
      if (header->value) {
         slab_free(parser->allocator, header->value);
         header->value = NULL;
      }
   }

   message->body_size = 0;
   message->current_body_size = 0;
}

int http_get_upper_part(http_parser *parser, char *buff, size_t buff_len) {
   if (parser->type == HTTP_PARSER_LINE) {
      int start_line_end = findstr(buff, buff_len, "\r\n", 2);
      if (start_line_end < 0) {
         goto free_this_trash;
      }
      start_line_end = start_line_end + 2;

      char tmp = buff[start_line_end - 2];
      buff[start_line_end - 2] = '\0';
      //   DEBUG_FUNC("start line: %s\n", buff);
      buff[start_line_end - 2] = tmp;

      parser->line_end = start_line_end;
      parser->type = HTTP_PARSER_HEADER;
   }

   if (parser->type == HTTP_PARSER_HEADER) {
      int headers_end = findstr(buff, buff_len, "\r\n\r\n", 4);
      if (headers_end < 0) {
         goto free_this_trash;
      }
      headers_end = headers_end + 4;

      char tmp = buff[headers_end - 4];
      buff[headers_end - 4] = '\0';
      //   DEBUG_FUNC("headers: %s\n", buff + parser->line_end);
      buff[headers_end - 4] = tmp;

      parser->headers_end = headers_end;
      parser->type = HTTP_PARSER_BODY;
   }

   return 0;
free_this_trash:
   return -1;
}

int http_validate_start_line(http_parser *parser, http_message *message) {
   char temp[parser->line_end + 1];
   strncpy(temp, parser->buff, parser->line_end);
   temp[parser->line_end] = '\0';

   char *method = strtok(temp, " ");
   char *path = strtok(NULL, " ");
   char *version = strtok(NULL, " ");

   if (!method || !path || !version) {
      DEBUG_FUNC("No string\n");
      goto free_this_trash;
   }

   http_method convert_method = http_parse_method(method);
   http_version convert_version = http_parse_version(version);

   if (convert_method == HTTP_METHOD_UNKNOWN) {
      DEBUG_FUNC("Unknown method!\n");
      goto invald_method;
   }

   if (convert_version == HTTP_VERSION_UNKNOWN) {
      DEBUG_FUNC("Unknown version!\n");
      goto invalid_version;
   }

   if (message) {
      request_line *request = &message->request;
      request->method = convert_method;
      request->target = path;
      request->version = convert_version;
   }

   DEBUG_FUNC("method: %d path: %s version: %d\n", convert_method, path, convert_version);
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
   int colon = findstr(line, line_len, ":", 1);
   if (colon < 0) {
      return - 1;
   }

   char *nameend = line + colon;
   char *value = nameend + 1;

   char *line_end = line + line_len; 
   while (value < line_end && isspace((unsigned char)*value)) {
      value++;
   }
   if (value >= line_end) {
      return -1;
   }

   size_t name_len = nameend - line;

   for (int index = 0; index < (int)name_len; index++) {
      if (isspace((unsigned char)line[index])) {
         return -1; 
      }
   }

   char write_name[HTTP_HEADER_NAME_SIZE];
   char write_value[HTTP_HEADER_VALUE_SIZE];

   strncpy(write_name, line, name_len);
   write_name[name_len] = '\0';

   strncpy(write_value, value, sizeof(write_value) - 1);
   write_value[sizeof(write_value) - 1] = '\0';

   if (header) {
      strncpy(header->name, write_name, HTTP_HEADER_NAME_SIZE);
      header->name[HTTP_HEADER_NAME_SIZE - 1] = '\0';
      strncpy(header->value, write_value, HTTP_HEADER_VALUE_SIZE);
      header->value[HTTP_HEADER_VALUE_SIZE - 1] = '\0';
   }

   return 0;
}

int http_headers_parse(http_parser *parser, http_message *message, char *headers, size_t headers_size) {
   int end_pos = findstr(headers, headers_size, "\r\n\r\n", 4);
   if (end_pos < 0) {
      return -1;
   }

   size_t header_size = end_pos + 4;
   size_t offset = 0;
   size_t count = 0;

   while (offset < header_size && count < HTTP_MAX_HEADERS) {
      char *line_start = headers + offset;
      int line_end = findstr(line_start, header_size - offset, "\r\n", 2);
      if (line_end < 0) {
         break;
      }

      if (line_end == 0) {
         offset += 2;
         continue;
      }

      char line[HTTP_HEADER_VALUE_SIZE];
      if (line_end > (int)sizeof(line)) {
         return -1;
      }

      memcpy(line, line_start, line_end);
      line[line_end] = '\0';

      http_header *header = &message->headers[count];

      if (!header->name ) {
         header->name = slab_malloc(parser->allocator, NULL, HTTP_HEADER_NAME_SIZE);
         if (!header->name) {
            return -1;
         }
      }

      if (!header->value) {
         header->value = slab_malloc(parser->allocator, NULL, HTTP_HEADER_VALUE_SIZE);
         if (!header->value) {
            slab_free(parser->allocator, header->name);
            return -1;
         }
      }

      int valid = http_validate_header(line, line_end, header);
      if (valid < 0) {
         return -1;
      }

      count = count + 1;
      offset += line_end + 2;
   }

   message->isUpperParsed = true;

   return 0;
}

int http_message_static_parse(http_parser *parser, http_message *message) {
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
      return 1;
   }
   return 0;
}

int http_message_chunk_parse(http_parser *parser, http_message *message) {
   size_t calculator = parser->headers_end + parser->pos;
   switch (parser->chunk_type) {
      case HTTP_CHUNK_TYPE_SIZE:
         int endpoint = findstr(parser->buff + calculator, parser->bytes_received - parser->pos, "\r\n", 2);
         if (endpoint < 0 || endpoint > 16) {
            return -1;
         }
         char temp[17];
         memcpy(temp, parser->buff + calculator, endpoint);
         temp[endpoint] = '\0';
         char *endptr = NULL; 
         parser->currentChunkSize = strtol(temp, &endptr, 16);
         if (endptr == temp) {
            return 0;
         }

         if (parser->currentChunkSize < 0) {
            return 0;
         }

         if (parser->currentChunkSize == 0) {
            DEBUG_FUNC("< 0  0 0 0 >\n");
            if (message->current_body_size > message->body_size) {
               http_message_exit(parser, parser->current_message_index);
               reset_parser(parser);
            }
            parser->chunk_type = HTTP_CHUNK_TYPE_DONE;
            message->body_size = message->current_body_size;
            DEBUG_FUNC("CHUNKED!\n");
            return 1;
         }

         parser->chunkSize = parser->currentChunkSize;
         parser->pos = parser->pos + endpoint + 2;
         parser->chunk_type = HTTP_CHUNK_TYPE_DATA;
         DEBUG_FUNC("yes1\n");
         break;
      case HTTP_CHUNK_TYPE_DATA:
         size_t can_read = parser->bytes_received - parser->pos; 
         size_t need_read = parser->currentChunkSize;
         size_t to_copy = can_read < need_read ? can_read : need_read;

         if (to_copy > 0) {
            memcpy(message->body + message->current_body_size, parser->buff + calculator, to_copy);
            message->current_body_size = message->current_body_size + to_copy;
            parser->pos = parser->pos + to_copy; 
            parser->currentChunkSize = parser->currentChunkSize - to_copy;
         }
         if (parser->currentChunkSize <= 0) {
            parser->chunk_type = HTTP_CHUNK_TYPE_CRLF;
            parser->currentChunkSize = 0;
         }
         DEBUG_FUNC("temp: %s\n", message->body);
         break;
      case HTTP_CHUNK_TYPE_CRLF:
         if ((parser->bytes_received - parser->pos) < 2) {
            return 0;
         }
         int chunk_end = findstr(parser->buff + calculator, parser->bytes_received - parser->pos, "\r\n", 2);
         if (chunk_end < 0) {
            return 0;
         }
         parser->pos = parser->pos + 2;
         parser->chunk_type = HTTP_CHUNK_TYPE_SIZE;
         parser->chunkSize = 0;
         DEBUG_FUNC("yes3\n");
         break;
      case HTTP_CHUNK_TYPE_DONE:
         DEBUG_FUNC("need to send the trasher!\n");
         return 1;
      default:
         return -1;
   }
   return 0;
}

http_parse_ret http_message_body_parse(http_parser *parser, http_message *message) {
   size_t body_size = message->body_size;
   if (!message->isActive) {
      return HTTP_PARSE_RET_FAILED;
   }

   if (!message->body) {
      return HTTP_PARSE_RET_FAILED;
   }

   if (message->isChunked) {
      if (message->current_body_size >= HTTP_BODY_MAX_SIZE) {
         return HTTP_PARSE_RET_FAILED;
      }
   } else {
      if (body_size <= 0 || body_size >= HTTP_BODY_MAX_SIZE) {
         return HTTP_PARSE_RET_FAILED;
      }
   }

   if (message->isReady) {
      return HTTP_PARSE_RET_EXTRA;
   }

   int ret = HTTP_PARSE_RET_FAILED;
   if (message->isChunked) {
      ret = http_message_chunk_parse(parser, message);
   } else {
      ret = http_message_static_parse(parser, message);
   }

   switch (ret) {
      case -1: return HTTP_PARSE_RET_FAILED;
      case 0: return HTTP_PARSE_RET_YET;
      default: return HTTP_PARSE_RET_OK;
   }

   return ret;
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

   size_t temp_size = 1024;
   char temp[temp_size];
   memset(temp, 0, temp_size);

   int content_length = http_string_parse(buff + parser->line_end, parser->headers_end - parser->line_end, "Content-Length:", temp, temp_size);
   int transfer_encoding = http_string_parse(buff + parser->line_end, parser->headers_end - parser->line_end, "Transfer-Encoding:", temp, temp_size);

   if (!parser->isChunked && transfer_encoding >= 0) {
      if (strcmp(temp, CHUNKED) == 0) {
         DEBUG_FUNC("TRANSFER_ENCODING: %s\n", temp);
         parser->isChunked = true;
         parser->chunk_type = HTTP_CHUNK_TYPE_SIZE;
      } 
   }

   size_t body_len = parser->isChunked ? HTTP_CHUNK_MAX_SIZE : (content_length > 0 ? content_length : 0);
   if (body_len <= 0) {
      goto isnt_demand;
   }

   int current_message_index = parser->current_message_index; 
   bool needInit = false;
   if (current_message_index < 0 || !parser->messages[current_message_index].isActive) {
      current_message_index = http_find_free_message(parser);
      if (current_message_index < 0) {
         return;
      }
      parser->current_message_index = current_message_index;
      needInit = true;
   }

   http_message *message = &parser->messages[current_message_index];
   //  request_line *request = &message->request;

   if (needInit) {
      ret = http_message_init(parser, current_message_index, HTTP_MSGTYPE_REQUEST, body_len);
      if (ret < 0) {
         goto isnt_demand;
         return;
      }

      if (HTTP_CHECK_HOST) {
         ret = http_check_on_host(parser->buff + parser->line_end, parser->headers_end - parser->line_end);
         if (ret < 0) {
            goto isnt_demand;
         }
      }

      if (message->request.method == HTTP_METHOD_UNKNOWN) {
         ret = http_start_line_parse(parser, message);
         if (ret < 0) {
            goto isnt_demand;
         }

      }

      if (!message->isUpperParsed) {
         ret = http_headers_parse(parser, message, parser->buff + parser->line_end, parser->headers_end - parser->line_end);
         if (ret < 0) {
            goto isnt_demand;
         }
      }
   }

   if (message->body == NULL) {
      DEBUG_FUNC("IS NULL!!!\n");
      return;
   }

   if (parser->bytes_received < (size_t)parser->headers_end) {
      return;
   }

   ret = http_message_body_parse(parser, message);
   DEBUG_FUNC("RET: : : %d\n", ret);
   if (ret == HTTP_PARSE_RET_OK) {
      message->isReady = true;
      message->body[message->current_body_size] = '\0';
      reset_parser(parser);
      DEBUG_FUNC("FINISHED!\n");
   } else if (ret == HTTP_PARSE_RET_FAILED) {
      http_message_exit(parser, parser->current_message_index);
      reset_parser(parser);
      DEBUG_FUNC("faileddeedede!\n");
      return;
   } else if (ret == HTTP_PARSE_RET_EXTRA) {
      reset_parser(parser);
      DEBUG_FUNC("extrararararar\n");
      return;
   } else if (ret == HTTP_PARSE_RET_YET) {
      DEBUG_FUNC("SESAEKASKEASKEKAS\n");
      return;
   }

   for (size_t index = 0; index < parser->messages_capacity; index++) {
      http_message *temp_msg = &parser->messages[index];
      if (temp_msg->isActive) {
         DEBUG_FUNC("index: %d msg: %s\n", (int)index, temp_msg->body);
      }
   }

   return;
isnt_demand:
   http_message_exit(parser, current_message_index);
   reset_parser(parser);
   return;
}

http_consume_result http_parser_consume(http_parser *parser, const char *message, size_t bytes_received) {
   struct slab *allocator = parser->allocator;
   if (!allocator) {
      DEBUG_FUNC("No allocator!\n");
      goto free_this_trash;
   }
   if (parser->type == HTTP_PARSER_DONE) {
      DEBUG_FUNC("STARTED!\n");
      parser->type = HTTP_PARSER_LINE;
   }
   DEBUG_FUNC("\n");
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

   reset_parser(parser);

   parser->messages_capacity = HTTP_MAX_MESSAGES;
   parser->messages_count = 0;

   parser->allocator = allocator;
   memset(parser->buff, 0, sizeof(parser->buff));

   for (size_t index = 0; index < parser->messages_capacity; index++) {
      //   http_message *message = &parser->messages[index];
      http_message_exit(parser, index);
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

void http_reset_parser(http_parser *parser) {
   parser->current_message_index = -1;

   parser->line_end = 0;
   parser->headers_end = 0;
   parser->body_end = 0;

   parser->buff_len = 0;
   parser->bytes_received = 0;
   parser->chunkSize = 0;
   parser->currentChunkSize = 0;
   parser->pos = 0;
   parser->isChunked = false;
   parser->chunk_type = HTTP_CHUNK_TYPE_DONE;
   parser->type = HTTP_PARSER_DONE;
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
