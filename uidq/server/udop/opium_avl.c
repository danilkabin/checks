#include "udop/opium_avl.h"
#include "core/opium_alloc.h"
#include "core/opium_core.h"
#include <stddef.h>
#include <stdio.h>

   opium_avl_t *
opium_avl_init(int key)
{
   opium_avl_t *node = opium_calloc(sizeof(opium_avl_t), NULL);
   if (!node) {
      printf("Fail!\n");
      return NULL;
   }

   node->right = NULL;
   node->left = NULL;
   node->key = key;
   node->height = 0;

   return node; 
}

   int
opium_avl_height(opium_avl_t *node)
{
   return node != NULL ? node->height : -1;
}

   int
opium_avl_height_diff(opium_avl_t *node)
{
   return node != NULL ? (opium_avl_height(node->left) - opium_avl_height(node->right)) : 0; 
}

   opium_avl_t *
opium_avl_min(opium_avl_t *node)
{
   opium_avl_t *temp = node;

   while (temp->left != NULL) {
      temp = temp->left;
   }

   return temp; 
}

/* Why do we need rotations in an AVL tree?
 *
 * For example, we have this tree and we need to find the X node. It`ll be a linear search O(N)
 * If you are looking for x:
 * - start with z -> go left
 * - finish with y -> go left again
 * - finally, x. Count of steps = 3. In general O(N)
 * 
 *        z
 *       /
 *      y
 *     /
 *    x
 * 
 * And if we turn around:
 * Now search for x:
 * - start with y -> go left -> immediately get to x
 *
 *        y
 *       / \
 *      x   z
 *
 * Count of steps: 2. O(log N).
 * This is not such a degenerate tree anymore, since when the elements increase 
 * it puts the bitch in the doggy style.
 *
 */

   opium_avl_t *
opium_avl_right_rotate(opium_avl_t *z)
{
   /* Before right rotation
    *
    *            z
    *           /
    *          y
    *         / \
    *        x   T3
    *             \
    *             ... 
    *
    */

   /* Save the left child (Y) of the current node (Z) - this will become the new root of the subtree */
   opium_avl_t *y = z->left;

   /* Save the right child (T3) of the parent node (Y) - this will become new left child of the previous root [Z] */
   opium_avl_t *T3 = y->right;

   /* Move our previous root [Z]. It becomes the right child of Y */ 
   y->right = z;

   /* Attach T3 as the left child of Z */
   z->left = T3;

   /* After right rotation
    *
    *        y
    *       / \
    *      x   z
    *         /
    *       T3
    *
    */

   /* Only [Z] and [Y] need height updates.
    * Because their child pointers were changed.
    *
    * - [Z] now has a left child [T3] instead of [Y]
    * - [Y] now has a left child [Z] instead of [T3]
    *
    * Nodes [X] and [T3] keep the same childrens, so their heights remain valid
    *
    */

   /* +1 accounts for the current node itself */

   z->height = (opium_max(opium_avl_height(z->right), opium_avl_height(z->left)) + 1);
   y->height = (opium_max(opium_avl_height(y->right), opium_avl_height(y->left)) + 1);

   return y; /* Return the new root */
}

/* Now left rotate implementation */

   opium_avl_t *
opium_avl_left_rotate(opium_avl_t *z)
{
   /* Before left rotation
    *
    *       z
    *        \
    *         y
    *        / \
    *      T3   x
    *
    */

   /* Save the right child (Y) of the current node (Z) - this will become the new root of the subtree */
   opium_avl_t *y = z->right;

   /* Save the left child (T3) of the parent node (Y) - this will become the new right child of the previous root [Z] */
   opium_avl_t *T3 = y->left;

   /* Move our previous root [Z]. It becomes the left child of Y */
   y->left = z;

   /* Attach T3 as the right child of Z */ 
   z->right = T3;

   /* After left rotation
    *
    *        y
    *       / \
    *      z   x
    *       \  
    *        T3
    *
    */

   /* Only [Z] and [Y] need height updates.
    * Because their child pointers were changed.
    *
    * - [Z] now has a left child [T3] instead of [Y]
    * - [Y] now has a left child [Z] instead of [T3]
    *
    * Nodes [X] and [T3] keep the same childrens, so their heights remain valid
    *
    */

   /* +1 accounts for the current node itself */

   z->height = (opium_max(opium_avl_height(z->right), opium_avl_height(z->left)) + 1);
   y->height = (opium_max(opium_avl_height(y->right), opium_avl_height(y->left)) + 1);

   return y; /* Return the new root */
}

   opium_avl_t *
opium_avl_right_left(opium_avl_t *z)
{

   /* Before right left rotation, the tree is unbalanced:
    *
    *      z
    *       \
    *        y
    *       /
    *      x
    *
    * Node Z has a right child Y, which in turn has a left-heavy subtree X.
    * This is right-left case in AVL terminology.
    *
    */

   /* Step 1: right rotate the right child (Y) of Z.
    * This transform the right-heavy subtree into a more balanced shape. 
    *      z
    *       \
    *        x
    *         \
    *          y
    */          

   z->right = opium_avl_right_rotate(z->right);

   /* Step 2: left rotate the root Z.
    * Now we rotate Z with its new right child (X) to fix the overall imbalance.
    *
    *        x
    *       / \
    *      z   y
    *
    */

   return opium_avl_left_rotate(z);
}

   opium_avl_t *
opium_avl_left_right(opium_avl_t *z)
{

   /* Before left right rotation, the tree is unbalanced:
    *
    *      z
    *     /
    *    y
    *     \ 
    *      x
    *
    * Node Z has a left child Y, which in turn has a right-heavy subtree X.
    * This is left-right case in AVL terminology
    */

   /* Step 1: left rotate the left child (Y) of Z.
    * This transform the left-heavy subtree into a more balanced shape. 
    *          z
    *         /
    *        x
    *       /
    *      y
    */

   z->left = opium_avl_left_rotate(z->left);

   /* Step 2: left rotate the root Z.
    * Now we rotate Z with its new right child (X) to fix the overall imbalance.
    *
    *        x
    *       / \
    *      y   z
    *
    */

   return opium_avl_right_rotate(z);
}

   opium_avl_t *
opium_avl_insert(opium_avl_t *node, int key)
{
   if (!node) {
      return opium_avl_init(key);
   }

   if (key > node->key) {
      node->right = opium_avl_insert(node->right, key);

   } else if (key < node->key) {
      node->left = opium_avl_insert(node->left, key);

   }

   node->height = (opium_max(opium_avl_height(node->right), opium_avl_height(node->left)) + 1);

   int balance = opium_avl_height_diff(node); 

   /* 1: Right Right
    * Condition: balance < -1 && key > node->right->key. What does it mean?
    * The right subtree is to tall. New key inserted into right subtree of right child.
    * Tree before the turn: [NUMBERS FOR EXAMPLE] INSERT KEY: 35
    *        z [10]                  y[20]
    *         \                     /    \
    *          y [20]        ->   z[10]   x[30]
    *           \                          \ 
    *            x [30]                     i[35]
    *    key[35] > y.right.key[30]
    */

   if (balance < -1 && key > node->right->key) {
      return opium_avl_left_rotate(node);
   }

   /* 2: Left Left
    * Condtion: balance > 1 && key < node->left->key. What does it mean?
    * The left subtree is to tall. New key inserted into left subtree of left child.
    * Tree before the turn: [NUMBERS FOR EXAMPLE] INSERT KEY: 5
    *             z [30]            y[20]
    *            /                 /    \
    *           y [20]       ->  x[10]   z[30] 
    *          /                /
    *         x [10]          i[5]
    *    key[5] < y.left.key[10]
    */

   if (balance > 1 && key < node->left->key) {
      return opium_avl_right_rotate(node);
   }

   /* 3: Right Left
    * Condition: balance < -1 && key < node->right->key. What does it mean?
    * The right subtree is to tall. New key isnserted into left subtree of right child.
    * Tree before the turn: [NUMBERS FOR EXAMPLE] INSERT KEY: 25
    *        z[10]         z[10]             x[25]
    *         \             \               /  \
    *          y[30]   ->    x[25]    -> z[10]  y[30]
    *         /               \              
    *        x[25]             y[30]         
    *       key[25] < y.key[30]  
    */

   if (balance < -1 && key < node->right->key) {
      node = opium_avl_right_left(node);      
   }

   /* 4: Left Right
    * Condition: balance > 1 && key > node.left.key. What does it mean?
    * The left subtree is to tall. New key inserted into right subtree of left child.
    * Tree before the turn: [NUMBERS FOR EXAMPLE] INSERT KEY: 15
    *         z[30]             z[30]           x[15]
    *        /                 /               /   \
    *       y[10]    ->      x[15]      ->   y[10] z[30]
    *        \              /            
    *         x[15]       y[10]        
    *        x[15] > y.key[10] 
    */

   if (balance > 1 && key > node->left->key) {
      node = opium_avl_left_right(node);
   }

   return node;
}

   opium_avl_t *
opium_avl_delete(opium_avl_t *node, int key)
{
   if (!node) {
      return NULL;
   }

   if (key > node->key) {
      node->right = opium_avl_delete(node->right, key);
   } else if (key < node->key) {
      node->left = opium_avl_delete(node->left, key);
   } else {

      /* <= 1 Chidren */

      if (!node->right || !node->left) {
         opium_avl_t *temp = node->right ? node->right : node->left;

         if (!temp) {
            temp = node;
            node = NULL;
         } else {
            *node = *temp;
         }

         opium_free(temp, NULL);
      } else {
         /* 2 Childrens */

         /* We want to delete 50. min(node[50].right) -> 60
          *       [FIRST]          [SECOND]     [THIRD]
          *        50              60            60
          *       /  \            /  \          /  \
          *     30    70     ->  30   70    -> 30   70
          *          /  \            /  \            \
          *        60    80         60   80           80
          */ 

         /* FIRST */
         opium_avl_t *temp = opium_avl_min(node->right);
         
         /* SECOND */
         node->key = temp->key;

         /* THIRD. DELETE THE SMALLEST IN THE SUBTREE [60 in picture] */
         node->right = opium_avl_delete(node->right, temp->key);
      }

   }

   if (!node) {
      return node;
   }

   node->height = (opium_max(opium_avl_height(node->right), opium_avl_height(node->left)) + 1);

   int balance = opium_avl_height_diff(node);

   if (balance > 1 && (opium_avl_height_diff(node->left) >= 0)) {
      return opium_avl_right_rotate(node);
   }

   if (balance < -1 && (opium_avl_height_diff(node->right) >= 0)) {
      return opium_avl_left_rotate(node);
   }

   if (balance > 1 && (opium_avl_height_diff(node->left) < 0)) {
      node = opium_avl_left_right(node);
   }

   if (balance < -1 && (opium_avl_height_diff(node->right) < 0)) {
      node = opium_avl_right_left(node);
   }

   return node;
}

   void
opium_avl_debug(opium_avl_t *node)
{
   if (!node) {
      return;
   }

   opium_avl_debug(node->left);
   printf(" %d ", node->key);
   opium_avl_debug(node->right);
}
