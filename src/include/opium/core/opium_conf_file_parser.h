#ifndef OPIUM_CONF_FILE_PARSER_INCLUDE_H
#define OPIUM_CONF_FILE_PARSER_INCLUDE_H

#include "core/opium_core.h"

typedef enum {
   OPIUM_COMMAND_SERVER_TYPE,
   OPIUM_COMMAND_WORKER_TYPE,
   OPIUM_COMMAND_LOCATION_TYPE,
   OPIUM_COMMAND_INVALID_TYPE
} opium_command_type_t;

typedef enum {
   OPIUM_TOKEN_STRING,
   OPIUM_TOKEN_NUMBER,
   OPIUM_TOKEN_INTEGER,
   OPIUM_TOKEN_DOUBLE,
   OPIUM_TOKEN_BOOLEAN
} opium_conf_token_type_t;

typedef struct {
  u_char                *start;
  u_char                *pos;
  u_char                *end;
  opium_uint_t           line;
} opium_conf_parse_ctx_t;

typedef struct {
   opium_string_t key;
   opium_string_t value;
   opium_conf_token_type_t type;
} opium_conf_token_t;

struct opium_command_s {
   opium_string_t        name;
   opium_command_type_t  type;
   char* (*set)(opium_conf_file_t *conf_file,
         struct opium_command_s *cmd,
         void *data);
   void                 *conf;
   opium_uint_t          offset;
};

typedef struct opium_conf_block_s {
   opium_string_t        name;
   opium_array_t         tokens;
   opium_array_t         children;
   opium_uint_t          line;
   struct opium_conf_block_s *parent;
} opium_conf_block_t;

struct opium_conf_file_s {
   u_char               *start;
   size_t                len;

   opium_array_t         tokens;
   opium_array_t         blocks;

   opium_uint_t          line;
   
   opium_conf_block_t   *root;
   opium_conf_block_t   *current;
}; 

#endif /* OPIUM_CONF_FILE_PARSER_INCLUDE_H  */
