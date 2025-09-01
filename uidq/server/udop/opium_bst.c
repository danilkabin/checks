#include "udop/opium_bst.h"
#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include <stdio.h>

   opium_bst_node_t *
opium_bst_node_init(int data)
{
   opium_bst_node_t *node = opium_calloc(sizeof(opium_bst_node_t), NULL);
   if (!node) {
      printf("Failed to allocate node.\n");
      return NULL;
   }

   node->right = NULL;
   node->left = NULL;
   node->data = data;

   return node;
}

   opium_bst_node_t *
opium_bst_node_getMax(opium_bst_node_t *root)
{
   if (root->right != NULL) {
      root = opium_bst_node_getMax(root->right);
   }

   return root;
}

   opium_bst_node_t *
opium_bst_node_find(opium_bst_node_t *root, int data)
{
   if (root == NULL) {
      return NULL;
   }

   if (data == root->data) {
      return root;
   } else if (data < root->data) {
      return opium_bst_node_find(root->left, data);
   } else {
      return opium_bst_node_find(root->right, data);
   }

   return root;
}

   opium_bst_node_t *
opium_bst_node_insert(opium_bst_node_t *root, int data)
{
   if (root == NULL) {
      root = opium_bst_node_init(data);

   } else if (data > root->data) {
      root->right = opium_bst_node_insert(root->right, data); 

   } else if (data < root->data) {
      root->left = opium_bst_node_insert(root->left, data); 
   }

   return root;
}

   opium_bst_node_t * 
opium_bst_node_remove(opium_bst_node_t *root, int data)
{
   opium_bst_node_t *temp = NULL;

   if (root == NULL) {
      return NULL;
   }

   if (data < root->data) {
      root->left = opium_bst_node_remove(root->left, data);
   } else if (data > root->data) {
      root->right = opium_bst_node_remove(root->right, data);
   } else {
      if (root->left == NULL && root->right == NULL) {
         opium_free(root, NULL);      
         return NULL;

      } else if (root->right == NULL) {
         temp = root;
         root = root->left;
         opium_free(temp, NULL);
         return root; 

      } else if (root->left == NULL) {
         temp = root;
         root = root->right;
         opium_free(temp, NULL);  
         return root;

      } else {
         temp = opium_bst_node_getMax(root->left);

         root->data = temp->data;
         root->left = opium_bst_node_remove(root->left, temp->data);
      }
   }

   return root;
}

int
opium_bst_node_height(opium_bst_node_t *root)
{
   if (root == NULL) {
      return 0;
   } else {
      int right_height = opium_bst_node_height(root->right);
      int left_height = opium_bst_node_height(root->left);

      if (right_height > left_height) {
         return (right_height + 1);
      } else {
         return (left_height + 1);
      }
   }
}

   void
opium_bst_node_debug1(opium_bst_node_t *node)
{
   if (node == NULL) {
      return;
   }

   opium_bst_node_debug1(node->left);
   printf("%d ", node->data);
   opium_bst_node_debug1(node->right);
}

   void
opium_bst_node_debug(opium_bst_node_t *node)
{
   opium_bst_node_debug1(node);
   printf("\n");
}

