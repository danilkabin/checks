#include "opium_rbtree.h"
#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include <stddef.h>
#include <stdio.h>

   int
opium_rbt_is_valid(opium_rbt_t *rbt)
{
   return rbt && rbt->pool;
}

   opium_rbt_node_t *
opium_rbt_min(opium_rbt_node_t *node, opium_rbt_node_t *sentinel)
{
   if (!node || node == sentinel) return sentinel;
   while (node->left != sentinel) {
      node = node->left;
   }
   return node;
}

   opium_rbt_t *
opium_rbt_init(size_t capacity, opium_log_t *log)
{
   if (capacity == 0) {
      opium_err(log, "Invalid parameters.\n"); 
      return NULL;
   }

   opium_rbt_t *rbt = opium_calloc(sizeof(opium_rbt_t), log);
   if (!rbt) {
      opium_err(log, "Failed to allocate rbt tree.\n"); 
      return NULL;
   }

   rbt->pool = opium_pool_create(capacity, sizeof(opium_rbt_node_t), log);
   if (!rbt->pool) {
      opium_err(log, "Failed to allocate rbt pool.\n"); 
      goto fail;
   }

   rbt->sentinel = opium_pool_get(rbt->pool, opium_pool_push(rbt->pool, NULL));
   if (!rbt->sentinel) {
      opium_err(log, "Failed to alloacte rbt sentinel.\n"); 
      goto fail;
   }

   opium_rbt_sentinel_init(rbt->sentinel);

   rbt->head = rbt->sentinel;
   rbt->log = log;

   return rbt;

fail:
   opium_rbt_exit(rbt);
   return NULL;
}

   void
opium_rbt_exit(opium_rbt_t *rbt)
{
   if (!opium_rbt_is_valid(rbt)) {
      return;
   }

   if (rbt->pool) {
      opium_pool_abort(rbt->pool);
      rbt->pool = NULL;
   }

   opium_free(rbt, NULL);
}


   opium_rbt_node_t *
opium_rbt_node_init(opium_rbt_t *rbt, opium_rbt_node_t *parent, opium_rbt_key_t key)
{
   opium_rbt_node_t *node = opium_pool_get(rbt->pool, opium_pool_push(rbt->pool, NULL));
   if (!node) {
      return NULL;
   }

   node->parent = parent;
   node->right = rbt->sentinel;
   node->left = rbt->sentinel;

   node->key = key;
   node->data = NULL;
   opium_rbt_red(node);

   return node;
}

   opium_rbt_node_t *
opium_rbt_right_rotate(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *y)
{
   opium_rbt_node_t *x = y->left;
   if (x == sentinel) {
      return y;
   }

   /* Right rotate around node (was parent in original)
    * Initial Situation
    *       y(B)
    *      / \
    *    x(R)  C
    *   / \
    *  A   B
    */

   y->left = x->right;
   if (x->right != sentinel) {
      x->right->parent = y;
   }

   x->parent = y->parent;

   if (y->parent == NULL) {
      rbt->head = x;
   } else if (y->parent->right == y) {
      y->parent->right = x;
   } else if (y->parent->left == y) {
      y->parent->left = x;
   }

   x->right = y;
   y->parent = x;

   /* Eventual Situation
    *
    *       x(B)
    *      / \
    *     A   y(R)
    *        / \
    *       B   C
    */

   return x;
}

   opium_rbt_node_t *
opium_rbt_left_rotate(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *y)
{
   opium_rbt_node_t *x = y->right;
   if (x == sentinel) {
      return y;
   }

   /* Initial Situation
    * Left rotate around P
    *    Y(B)
    *   / \
    *  A   X(R)
    *     / \
    *    B   C
    */

   y->right = x->left;
   if (x->left != sentinel) {
      x->left->parent = y;
   }

   x->parent = y->parent;

   if (y->parent == NULL) {
      rbt->head = x;
   } else if (y->parent->right == y) {
      y->parent->right = x;
   } else if (y->parent->left == y) {
      y->parent->left = x;
   }

   x->left = y;
   y->parent = x;

   /* Eventual Situation
    *       X(B)
    *      / \
    *   Y(R)  C
    *   / \
    *  A   B
    */

   return x;
}

   opium_rbt_node_t *
opium_rbt_node_insert(opium_rbt_t *rbt, opium_rbt_node_t **root, opium_rbt_key_t key)
{
   opium_rbt_node_t *sentinel = rbt->sentinel;
   opium_rbt_node_t *parent = NULL;
   opium_rbt_node_t *temp = *root;

   while (temp != sentinel) {
      parent = temp;
      if (key < temp->key) {
         temp = temp->left;
      } else if (key > temp->key) {
         temp = temp->right;
      } else {
         return NULL;
      }
   }

   opium_rbt_node_t *node = opium_rbt_node_init(rbt, NULL, key);
   if (!node) {
      return NULL;
   }

   if (*root == sentinel) {
      node->parent = NULL;
      node->right = sentinel;
      node->left = sentinel;
      opium_rbt_black(node);
      *root = node;

      return node;
   }

   node->parent = parent;
   if (key < parent->key) {
      parent->left = node;
   } else {
      parent->right = node;
   }

   return node;
}

void
opium_rbt_insert_data(opium_rbt_node_t *node, void *data)
{
    if (!node) return;
    node->data = data;
}

   opium_rbt_node_t *
opium_rbt_insert(opium_rbt_t *rbt, opium_rbt_key_t key, void *data)
{
   if (!rbt) {
      return NULL;
   }

   opium_rbt_node_t **root = &rbt->head;

   opium_rbt_node_t *node = opium_rbt_node_insert(rbt, root, key); 
   if (!node) {
      return NULL;
   }

   /* Now we have already inserted the node,
    * so we need to balance the tree.
    * The loop continues while:
    *   1. We are not at the root node
    *   2. The parent of the current node is red
    *
    *   Reasoning:
    *   - If the parent is red, we violate the rule:
    *    "A red node cannot have a red parent".
    *   Therefore, rebalancing is required until this condition is fixed.
    */

   opium_rbt_node_t *temp;

   while (node != *root && opium_rbt_is_red(node->parent)) {
      if (node->parent->parent == NULL) break;
      /* Regarding checks #1
       * Thus, we need to determine on which side the parent is located.
       * Why? Because the balancing logic is mirrored for left and right cases.
       *
       * temp = uncle of our node.
       * The location of Parent and Uncle is just mirrored, depending on where the parent is.
       * If you don't know who the uncle is, look at the picture:
       *
       *        G          G
       *       / \   or   / \
       *      P   U      U   P
       *      |              |
       *      N              N
       *
       * The uncle is simply the parent's sibling (or the parent's "brother"),
       * whichever explanation is more convenient for you.
       */

      /* Regarding checks #2
       * We have added a new node to the subtree above (with insert)
       * The node is red by default (with some exceptions).
       * And it`s parent node is too red, but rb tree rule #4 is violated.
       * <A red node cannot have a red parent>
       * 
       * If uncle is red -> the grandParent has two red children.
       * To fix the situation:
       * - Parent (P) is painted black.
       * - Uncle (U) is painted black.
       * - grandParent (G) is painted red.
       * 
       * ? Why do we do this:
       * Now P and U are okay - they are black, the "two reds in row" rule isn`t violated.
       * G was repainted red because the balance "on black heights" must be maintained.
       * If g was the root and turned red ->
       * later the loop will pick up node G and repaint it black
       * (at the end it always make root = black)
       */

      /* Regarding checks #3
       * Symmetry check:
       * We check the relative position of the new node.
       */

      if (node->parent == node->parent->parent->right) { // #1
         temp = node->parent->parent->left; // Uncle

         if (opium_rbt_is_red(temp)) { // #2
            opium_rbt_black(node->parent);
            opium_rbt_black(temp);
            opium_rbt_red(node->parent->parent);
            node = node->parent->parent;

         } else {
            if (node == node->parent->left) { // #3
               /* Right Left case */
               node = node->parent;
               opium_rbt_right_rotate(rbt, rbt->sentinel, node);
            }

            /* Right Right case */
            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_left_rotate(rbt, rbt->sentinel, node->parent->parent);
         }

      } else { // #1
         temp = node->parent->parent->right; // Uncle

         if (opium_rbt_is_red(temp)) { // #2
            opium_rbt_black(node->parent);
            opium_rbt_black(temp);
            opium_rbt_red(node->parent->parent);
            node = node->parent->parent;

         } else {
            if (node == node->parent->right) { // #3
               /* Left Right case */
               node = node->parent;
               opium_rbt_left_rotate(rbt, rbt->sentinel, node);
            }

            /* Left Left case */
            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_right_rotate(rbt, rbt->sentinel, node->parent->parent);
         }

      }

   }

   node->data = data;

   opium_rbt_black(*root);

   return node;
}

   void
opium_rbt_delete(opium_rbt_t *rbt, opium_rbt_key_t key)
{
   if (!rbt || !opium_rbt_is_valid(rbt)) {
      return;
   }

   opium_rbt_node_t **root = &rbt->head;
   opium_rbt_node_t *sentinel = rbt->sentinel;
   opium_rbt_node_t *node = rbt->head;
   opium_rbt_node_t *parent, *sibling;

   /* Find the node to delete */
   while (node != sentinel) {
      if (node->key == key) {
         break;
      }
      if (key > node->key) {
         node = node->right;
      } else {
         node = node->left;
      }
   }

   if (node == sentinel) {
      return; /* Key not found */
   }

   /* Find the node to physically remove (successor or node itself) */
   opium_rbt_node_t *to_delete = node;
   int is_red = opium_rbt_is_red(node);

   if (node->left != sentinel && node->right != sentinel) {
      /* Node has two children, find the successor (leftmost of right subtree) */
      to_delete = node->right;
      while (to_delete->left != sentinel) {
         to_delete = to_delete->left;
      }
      /* Swap keys and data */
      node->key = to_delete->key;
      node->data = to_delete->data;
      is_red = opium_rbt_is_red(to_delete);

   }

   /* Determine the replacement child */
   opium_rbt_node_t *child = (to_delete->left != sentinel) ? to_delete->left : to_delete->right;

   /* Update parent-child links */
   parent = to_delete->parent;
   if (child != sentinel) {
      child->parent = parent;
   }

   if (to_delete == *root) {
      *root = child;
      if (child != sentinel) {
         opium_rbt_black(child); /* Root must be black */
      }
      opium_pool_popit(rbt->pool, to_delete);
      return;
   } else if (to_delete == parent->left) {
      parent->left = child;
   } else {
      parent->right = child;
   }

   /* If the deleted node or its child is red, simply color the child black */
   if (is_red || (child != sentinel && opium_rbt_is_red(child))) {
      if (child != sentinel) {
         opium_rbt_black(child);
      }
      opium_pool_popit(rbt->pool, to_delete);
      opium_rbt_black(*root); /* Ensure root is black */
      return;
   }

   /* Fix double black condition */
   opium_rbt_node_t *current = child;
   while (current != *root && (current == sentinel || opium_rbt_is_black(current))) {
      parent = current == sentinel ? parent : current->parent;
      if (current == parent->left || current == sentinel) {
         sibling = parent->right;

         if (opium_rbt_is_red(sibling)) {
            /* Case 1: Sibling is red */
            opium_rbt_red(parent);
            opium_rbt_black(sibling);
            opium_rbt_left_rotate(rbt, sentinel, parent);
            sibling = parent->right;
         }

         if ((sibling->left == sentinel || opium_rbt_is_black(sibling->left)) &&
               (sibling->right == sentinel || opium_rbt_is_black(sibling->right))) {
            /* Case 2: Sibling is black with two black children */
            opium_rbt_red(sibling);
            if (opium_rbt_is_red(parent)) {
               opium_rbt_black(parent);
               break; /* Double black resolved */
            }
            current = parent; /* Propagate double black up */
         } else {
            if (sibling->right == sentinel || opium_rbt_is_black(sibling->right)) {
               /* Case 3: Sibling is black, left child is red, right child is black */
               opium_rbt_red(sibling);
               opium_rbt_black(sibling->left);
               opium_rbt_right_rotate(rbt, sentinel, sibling);
               sibling = parent->right;
            }
            /* Case 4: Sibling is black, right child is red */
            opium_rbt_copy_color(sibling, parent);
            opium_rbt_black(parent);
            opium_rbt_black(sibling->right);
            opium_rbt_left_rotate(rbt, sentinel, parent);
            current = *root; /* Terminate loop */
         }
      } else {
         sibling = parent->left;

         if (opium_rbt_is_red(sibling)) {
            /* Case 1: Sibling is red (mirrored) */
            opium_rbt_red(parent);
            opium_rbt_black(sibling);
            opium_rbt_right_rotate(rbt, sentinel, parent);
            sibling = parent->left;
         }

         if ((sibling->right == sentinel || opium_rbt_is_black(sibling->right)) &&
               (sibling->left == sentinel || opium_rbt_is_black(sibling->left))) {
            /* Case 2: Sibling is black with two black children (mirrored) */
            opium_rbt_red(sibling);
            if (opium_rbt_is_red(parent)) {
               opium_rbt_black(parent);
               break; /* Double black resolved */
            }
            current = parent; /* Propagate double black up */
         } else {
            if (sibling->left == sentinel || opium_rbt_is_black(sibling->left)) {
               /* Case 3: Sibling is black, right child is red, left child is black (mirrored) */
               opium_rbt_red(sibling);
               opium_rbt_black(sibling->right);
               opium_rbt_left_rotate(rbt, sentinel, sibling);
               sibling = parent->left;
            }
            /* Case 4: Sibling is black, left child is red (mirrored) */
            opium_rbt_copy_color(sibling, parent);
            opium_rbt_black(parent);
            opium_rbt_black(sibling->left);
            opium_rbt_right_rotate(rbt, sentinel, parent);
            current = *root; /* Terminate loop */
         }
      }
   }

   /* Ensure the current node is black */
   if (current != sentinel) {
      opium_rbt_black(current);
   }

   opium_rbt_black(*root); /* Ensure root is black */
   opium_pool_popit(rbt->pool, to_delete);
}

   void
opium_rbt_debug(opium_rbt_t *rbt, opium_rbt_node_t *sentinel, opium_rbt_node_t *node)
{
   if (!rbt || !sentinel || !node || node == sentinel) {
      return;
   }

   opium_rbt_debug(rbt, sentinel, node->left);
   printf("%lld%s ", (long long)node->key, opium_rbt_is_red(node) ? "R" : "B");
   opium_rbt_debug(rbt, sentinel, node->right);
}
