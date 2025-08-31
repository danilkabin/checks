#include "opium_rbtree.h"
#include "core/opium_alloc.h"
#include "core/opium_core.h"
#include "core/opium_hash.h"
#include "core/opium_log.h"
#include <stdio.h>

static void opium_rbt_rotate_right(opium_rbt_node_t **head, opium_rbt_node_t *nill, opium_rbt_node_t *node);
static void opium_rbt_rotate_left(opium_rbt_node_t **head, opium_rbt_node_t *nill, opium_rbt_node_t *node);

   int
opium_rbt_isvalid(opium_rbt_t *rbtree) 
{
   return rbtree && rbtree->initialized == 1;
}

   opium_rbt_t *
opium_rbt_create(opium_rbt_node_t *nill, opium_rbt_ins_func_t insert, opium_log_t *log)
{
   opium_rbt_t *rbtree = opium_calloc(sizeof(opium_rbt_t), log);
   if (!rbtree) {
      opium_err(log, "Failed to allocate rbtree table.\n");
      return NULL;
   }
   rbtree->initialized = 1;

   if (opium_rbt_init(rbtree, nill, insert, log) != OPIUM_RET_OK) {
      opium_err(log, "Failed to initialize rbtree table.\n");
      opium_rbt_abort(rbtree);
      return NULL;
   }

   return rbtree;
}

   void 
opium_rbt_abort(opium_rbt_t *rbtree) 
{
   if (!opium_rbt_isvalid(rbtree)) {
      return;
   }

   opium_rbt_exit(rbtree);
   rbtree->initialized = 0;
   opium_free(rbtree, NULL);
}

   int
opium_rbt_init(opium_rbt_t *rbtree, opium_rbt_node_t *nill, opium_rbt_ins_func_t insert, opium_log_t *log)
{

   opium_rbt_nill_init(nill);
   rbtree->head = nill;
   rbtree->nill = nill;
   rbtree->insert = insert;

   rbtree->log = log;

   return OPIUM_RET_OK;
}

   void
opium_rbt_exit(opium_rbt_t *rbtree)
{

}

   static int
opium_rbt_compare(opium_rbt_node_t *node1, opium_rbt_node_t *node2)
{
   if (node1->data < node2->data) {
      return 1;
   } else if (node1->data > node2->data) {
      return -1;
   } else {
      return 0;
   }
}

   void
opium_rbt_insert1(opium_rbt_node_t *head, opium_rbt_node_t *nill, opium_rbt_node_t *node)
{
   opium_rbt_node_t *current = head;
   opium_rbt_node_t *parent = NULL;

   while (current != nill) {
      parent = current;
      if (current->data > node->data) {
         current = current->left;
      } else if (current->data < node->data) {
         current = current->right;
      } else {
         return;
      }
   }

   if (parent->data > node->data) {
      parent->left = node;
   } else {
      parent->right = node;
   }

   node->parent = parent;
   node->right = nill;
   node->left = nill;

   opium_rbt_red(node);

   printf("parent: %d\n", parent->data);

}

   void 
opium_rbt_insert(opium_rbt_t *rbtree, opium_rbt_node_t *node)
{
   opium_rbt_node_t **head = &rbtree->head;
   opium_rbt_node_t *nill = rbtree->nill;
   opium_rbt_node_t *uncle = NULL;

   if (*head == nill) {
      opium_debug(rbtree->log, "First!\n");
      node->parent = NULL;
      node->right = nill;
      node->left = nill;

      opium_rbt_black(node);

      *head = node;

      return;
   }

   rbtree->insert(*head, nill, node);

   while (node->parent && opium_rbt_is_red(node->parent)) {
      if (node->parent == node->parent->parent->left) {
         uncle = node->parent->parent->right;

         if (opium_rbt_is_red(uncle)) {
            opium_rbt_black(node->parent);
            opium_rbt_black(uncle);
            opium_rbt_red(node->parent->parent);

            node = node->parent->parent;
         } else {
            if (node == node->parent->right) {
               node = node->parent;
               opium_rbt_rotate_left(head, nill, node);
            }

            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_rotate_right(head, nill, node->parent->parent);
         }

      } else {
         uncle = node->parent->parent->left;

         if (opium_rbt_is_red(uncle)) {
            opium_rbt_black(node->parent);
            opium_rbt_black(uncle);
            opium_rbt_red(node->parent->parent);

            node = node->parent->parent;
         } else {
            if (node == node->parent->left) {
               node = node->parent;
               opium_rbt_rotate_right(head, nill, node);
            }

            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_rotate_left(head, nill, node->parent->parent);
         }

      }

}

}

   static void
opium_rbt_rotate_right(opium_rbt_node_t **head, opium_rbt_node_t *nill, opium_rbt_node_t *node)
{
   opium_rbt_node_t *temp = node->left;

   node->left = temp->right;

   if (temp->right != nill) {

   }
}

   static void 
opium_rbt_rotate_left(opium_rbt_node_t **head, opium_rbt_node_t *nill, opium_rbt_node_t *node)
{
   opium_rbt_node_t *parent = node->right;

   node->right = parent->left;

   if (parent->left != nill) {
      parent->left->parent = node;
   }

   parent->parent = node->parent;

   if (node == *head) {
      *head = parent;

   } else if (node == node->parent->left) {
      node->parent->left = parent;

   } else {
      node->parent->right = parent;

   }

   parent->left = node;
   node->parent = parent;
}

void opium_rbt_debug(opium_rbt_node_t *node, opium_rbt_node_t *nill, int depth) {
   if (node == nill || !node) return;

   opium_rbt_debug(node->right, nill, depth + 1);

   for (int i = 0; i < depth; i++) {
      printf("   ");
   }
   printf("%d (%s)\n", node->data, opium_rbt_is_black(node) ? "B" : "R");

   opium_rbt_debug(node->left, nill, depth + 1);
}
