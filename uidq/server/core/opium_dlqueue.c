#include "opium_dlqueue.h"
#include "core/opium_core.h"
#include "core/opium_list.h"
#include <stdio.h>

   void
opium_dlqueue_init(opium_dlqueue_t *queue)
{
   OPIUM_INIT_LIST_HEAD(&queue->head);
}

   int
opium_dlqueue_is_empty(opium_dlqueue_t *queue)
{
   return opium_list_empty(&queue->head);
}

   opium_dlqueue_node_t *
opium_dlqueue_first(opium_dlqueue_t *queue)
{
   if (opium_dlqueue_is_empty(queue)) {
      return NULL;
   }

   return opium_list_entry(queue->head.next, opium_dlqueue_node_t, head);
}

   opium_dlqueue_node_t *
opium_dlqueue_last(opium_dlqueue_t *queue)
{
   if (opium_dlqueue_is_empty(queue)) {
      return NULL;
   }

   return opium_list_entry(queue->head.prev, opium_dlqueue_node_t, head);
}

   void
opium_dlqueue_push(opium_dlqueue_t *queue, opium_dlqueue_node_t *node)
{
   opium_list_add_tail(&node->head, &queue->head);
}

   opium_dlqueue_node_t *
opium_dlqueue_pop(opium_dlqueue_t *queue)
{
   if (opium_list_empty(&queue->head)) {
      return NULL;
   }

   opium_dlqueue_node_t *node = opium_list_first_entry(&queue->head, opium_dlqueue_node_t, head);
   opium_list_del(&node->head);
   return node;
}

   void
opium_dlqueue_debug(opium_dlqueue_t *queue)
{
   struct opium_list_head *pos;
   printf("[");
   opium_list_for_each(pos, &queue->head) {
      opium_dlqueue_node_t *node = opium_list_entry(pos, opium_dlqueue_node_t, head); 
      printf(" %ld", node->key);
   }
   printf("]\n");
}

static opium_dlqueue_node_t *
opium_queue_middle(opium_dlqueue_t *queue)
{

   struct opium_list_head *slow = &queue->head;
   struct opium_list_head *fast = &queue->head;

   while (slow->next && fast->next->next) {
      slow = slow->next;
      fast = fast->next->next;
   }

   opium_dlqueue_node_t *node = opium_list_entry(slow, opium_dlqueue_node_t, head);

   return node;
}

void
opium_queue_sort(opium_dlqueue_t *queue, int (*cmp)(opium_dlqueue_t *, opium_dlqueue_t *))
{
   
}
