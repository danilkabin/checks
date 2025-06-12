#include "http.h"
#include "pool.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void *getLineType(http_message *message) {
   return message->type == HTTP_MSGTYPE_RESPONSE ? (void*)&message->start_line.status :
      message->type == HTTP_MSGTYPE_REQUEST ? (void*)&message->start_line.request :
      NULL;
}

void http_message_init() {

}



http_parser *http_parser_init() {
   int ret;
   http_parser *parser = malloc(sizeof(http_parser));

   if (!parser) {
      printf("Parser Initialization failed!\n");
      goto free_this_trash;
   }

   parser->buff_length = sizeof(parser->buffer);
   parser->content_length = 0;
   parser->buff_length = 0;
   parser->type = HTTP_PARSER_DONE;

   return parser;
free_parser:
   free(parser);
free_this_trash:
   return NULL;
}
