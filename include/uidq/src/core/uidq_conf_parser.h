#ifndef UIDQ_CONF_PARSER_H
#define UIDQ_CONF_PARSER_H

#include "core/uidq_listhead.h"
#include "core/uidq_log.h"
#include "core/uidq_slab.h"
#include <stdio.h>

typedef enum {
   UIDQ_PIDR_RETURN_EMPTY,
   UIDQ_PIDR_RETURN_OK,
   UIDQ_PIDR_RETURN_ERR,
} uidq_pidr_ret_t;

typedef enum {
    UIDQ_PIDR_TYPE_NULL,
    UIDQ_PIDR_TYPE_BOOL,
    UIDQ_PIDR_TYPE_INT,
    UIDQ_PIDR_TYPE_DOUBLE,
    UIDQ_PIDR_TYPE_STR,
    UIDQ_PIDR_TYPE_ARRAY, 
    UIDQ_PIDR_TYPE_OBJECT,
} uidq_pidr_type_t;

typedef struct {
   int name_off;
   int key;
} uidq_pidr_data_t;

typedef struct uidq_pidr_node {
   uidq_pidr_data_t data;
   uidq_pidr_type_t type;
   struct uidq_list_head head;
   union {
      int bool_val;
      int int_val;
      double doube_val;
      char *str_val;
      struct uidq_pidr_node *array;
      struct uidq_pidr_node *object; 
   };
} uidq_pidr_node_t;

typedef struct {
   uidq_pidr_node_t root;
   uidq_slab_t *nodes;
   uidq_log_t *log;
} uidq_pidr_t;

// API

uidq_pidr_t *uidq_pidr_init(const char *str, uidq_log_t *log);
void uidq_pidr_exit(uidq_pidr_t *pidr);

#endif // UIDQ_CONF_PARSER_H
