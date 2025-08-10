#include "uidq_conf_parser.h"
#include "core/uidq_alloc.h"
#include "core/uidq_listhead.h"
#include "core/uidq_log.h"
#include "core/uidq_slab.h"
#include "core/uidq_types.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uidq_pidr_node_t *uidq_pidr_node_create(uidq_pidr_t *pidr) {
   uidq_pidr_node_t *node = uidq_slab_alloc_v_init(&pidr->nodes, NULL, sizeof(uidq_pidr_node_t));
   if (!node) {
      uidq_err(pidr->log, "Failed to find a node.\n");
      return NULL;
   }

   return node; 
}

static uint8_t *uidq_pidr_skip_comms(uint8_t *str) {
   while (*str) {
      if (*str == '#') {
         while (*str && *str != '\n') str++;
      } else if (*str == '/' && str[1] == '*') {
         str = str + 2;
         while (*str && !(*str == '*' && str[1] == '/')) str++;
         if (*str) str++;
      } else if (*str && *str == '/' && str[1] == '/') {
         str = str + 2;
         while (*str && *str != '\n') str++;
      } else if (*str <= 32) {
         str++;
      } else {
         break;
      }
   }
   return str;
}

static uint8_t *uidq_pidr_find_symb(uint8_t *str, char ch) {
   str = uidq_pidr_skip_comms(str);
   while (*str) {
      if (*str == ch) {
         break;
      }
      str++;
   }
   return str;
}

// Checks if the character is a string delimiter (double quote)
bool uidq_pidr_is_string(char ch) {
   return ch == '"';
}

// Checks if the character is a digit (0-9)
bool uidq_pidr_is_number(char ch) {
   return (ch >= '0' && ch <= '9');
}

// Checks if the character is the start of an object '{'
bool uidq_pidr_is_object(char ch) {
   return ch == '{';
}

// Checks if the character is the end of an object '}'
bool uidq_pidr_is_object_end(char ch) {
   return ch == '}';
}

// Checks if the character is the start of an array '['
bool uidq_pidr_is_array(char ch) {
   return ch == '[';
}

// Checks if the string starts with "true" or "false"
bool uidq_pidr_is_boolean(char *str) {
   return (strncmp(str, "true", 4) == 0 || strncmp(str, "false", 5) == 0);
}

// Checks if the string starts with "null"
bool uidq_pidr_is_null(char *str) {
   return (strncmp(str, "null", 4) == 0);
}

static bool uidq_pidr_parse_object(uidq_pidr_t *pidr, uint8_t **str) {
   *str = uidq_pidr_find_symb(*str, '{');
   if (str) {
      *str = *str + 1;
      *str = uidq_pidr_skip_comms(*str);
   } 
   uidq_debug(pidr->log, "start at: %s\n", *str);

   if (**str == '}') {
      pidr->root.type = UIDQ_PIDR_TYPE_OBJECT;
      *str = *str + 1;
      uidq_debug(pidr->log, "this trash is empty\n");
      return false;
   }

   uint8_t *start = *str;
   int depth = 1;

   while (**str) { 
      if (**str && **str == '}' && depth == 0) {
         break;
      }

      if (uidq_pidr_is_object(**str)) {
         depth = depth + 1;
      } else if (uidq_pidr_is_object_end(**str)) {
         depth = depth - 1;
         if (depth == 0) {
            *str = *str + 1;
            break;
         }

      }


      *str = *str + 1;
   };

   size_t size = *str - start - 1;
   uidq_debug(pidr->log, "size: %zu\n", size);

   char buff[size + 1];
   buff[size] = '\0';
   memcpy(buff, start, size);

   uidq_debug(pidr->log, "DSGKFJSDGJODSJG SDOGJK SDOGJSDOPG: %s\n", buff);

   for (int index = 0; index < 10; index++) {
      uidq_pidr_node_t *nodochka = uidq_pidr_node_create(pidr);

   }

   return true;
}

uidq_pidr_t *uidq_pidr_init(const char *str, uidq_log_t *log) {
   if (!log) {
      return NULL;
   }

   // Allocate pidr structure and nodes slab 
   uidq_pidr_t *pidr = uidq_calloc(sizeof(uidq_pidr_t)); 
   if (!pidr) { 
      uidq_err(log, "Failed to allocate pidr struct.\n");
      return NULL;
   }

   pidr->nodes = uidq_slab_create_and_init(512, sizeof(uidq_pidr_node_t)); 
   if (!pidr->nodes) {
      uidq_err(log, "Failed to allocate nodes slab.\n"); 
      return NULL; 
   }

   UIDQ_INIT_LIST_HEAD(&pidr->root.head);
   pidr->log = log;

   uidq_debug(log, "log: %s\n", str); 

   uint8_t *new_str = (uint8_t*)str; 
   new_str = uidq_pidr_skip_comms(new_str); 

   uidq_debug(log, "new log: %s\n", new_str);

   uidq_pidr_parse_object(pidr, &new_str);

   return pidr;
}

void uidq_pidr_exit(uidq_pidr_t *pidr) {
   if (!pidr) return;

   free(pidr);
}
