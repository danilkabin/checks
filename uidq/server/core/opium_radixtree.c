#include "opium_radixtree.h"
#include "core/opium_alloc.h"
#include "core/opium_log.h"
#include <stdio.h>
#include <sys/types.h>

#define opium_radix_place_char(node, idx) ((node)->data + (idx))
#define opium_radix_place_node(node, idx) ((opium_radix_node_t **)((node)->data + (node)->size) + idx) 

   static opium_radix_node_t *
opium_radix_node_init(size_t children, int datafield)
{
   size_t size = sizeof(opium_radix_node_t) 
      + children 
      + opium_radix_padding(children)
      + sizeof(opium_radix_node_t*)*children;

   if (datafield) {
      size = size + sizeof(void*);
   }

   opium_radix_node_t *node = opium_calloc(size, NULL);
   if (!node) {
      return NULL;
   }

   node->iskey = 0;
   node->isnull = 0;
   node->iscompr = 0;
   node->size = children;
    printf("[DEBUG] node=%p, node->data=%p, end of block ~%p\n", 
           node, node->data, (void*)((char*)node + size));
   return node;
}

   opium_radix_t *
opium_radix_init(opium_log_t *log)
{
   opium_radix_t *radix = opium_calloc(sizeof(opium_radix_t), log);
   if (!radix) {
      opium_err(log, "Failed to allocate radix tree.\n");
      return NULL;
   }

   radix->initialized = 1;
   radix->ndnm = 0;
   radix->elnm = 0;
   radix->log = log;

   radix->head = opium_radix_node_init(0, 0);
   if (!radix->head) {
      opium_free(radix, NULL);
      return NULL;
   }

   return radix;
}

   int 
opium_radix_insert(opium_radix_t *radix, u_char *s, size_t len, void *data,
      void **old)
{
   opium_radix_node_t *current = radix->head;

   for (size_t index = 0; index < len; index++) {
      u_char ch = s[index];

      opium_radix_node_t *child = NULL;

      for (size_t jindex = 0; jindex < current->size; jindex++) {
         if (current->data[jindex] == ch) {
            child = *opium_radix_place_node(current, jindex);
            break;
         }
      }

      if (child) {
         printf("child: %s\n", child->data);
      }

      printf("current before: %p, %s\n", current, current->data);

      if (!child) {
         child = opium_radix_node_init(0, 0); /* Create new node */
         child->data[current->size] = ch;     /* Put new character */
         *opium_radix_place_node(current, current->size) = child;

         current->size = current->size + 1;
      }

      current = child;
      printf("current after: %p, %s\n", current, current->data);
   }

   current->iskey = 1;

   return -1;
}
