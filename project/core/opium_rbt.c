/* opium_rbt.c 
 * Red-Black Tree Properties
 * The following rules enforce the red-black tree balance:
 *    1) Each node is either red or black.
 *    2) The root is black
 *    3) All sentinel leaves are black.
 *    4) A red node must not have red children.
 *    5) All paths from a node to the leaves below contain the same number of black nodes.
 *
 */

#include "core/opium_core.h"
   
   int 
opium_rbt_init(opium_rbt_t *rbt, opium_log_t *log)
{
   assert(rbt != NULL);

   /* Slab is nice because all the nodes are the same size */
   opium_slab_init(&rbt->slab, sizeof(opium_rbt_node_t), log);

   /*
    * Sentinel (or NIL) is a fictitious node that denotes the 'end' of a tree.
    * It is used to simplify the logic: instead of NULL, a sentinel is used everywhere.
    */
   rbt->sentinel = opium_slab_alloc(&rbt->slab);
   if (!rbt->sentinel) {
      opium_log_err(rbt->log, "Failed to allocate sentinel in red black tree.\n");
      return OPIUM_RET_ERR;
   }

   opium_rbt_sentinel_init(rbt->sentinel);

   rbt->head = rbt->sentinel;
   rbt->log = log;

   return OPIUM_RET_OK;
}

   void
opium_rbt_exit(opium_rbt_t *rbt)
{
   assert(rbt != NULL);

   opium_slab_exit(&rbt->slab);

   rbt->head = rbt->sentinel = NULL;
   rbt->log = NULL;
}

   static opium_rbt_node_t *
opium_rbt_node_init(opium_rbt_t *rbt, opium_rbt_node_t *parent, opium_rbt_key_t key)
{
   opium_rbt_node_t *node = opium_slab_alloc(&rbt->slab);
   if (!node) {
      return NULL;
   }

   node->parent = parent;
   node->right = rbt->sentinel;
   node->left = rbt->sentinel;

   /* data is initially NULL; it can be set later via opium_rbt_insert_data. */
   node->key = key;
   node->data = NULL;

   /*
    * Why is red the default? The new node is red to minimize black-height 
    * violations (rule 5). If black is inserted, the black-height may change
    * along the path, requiring more fixes. Red only violates only rule 4,
    * which is easier to fix.
    *
    * Why is it easier to fix? Violating rule 4 requiring checking only the local
    * structure (node, parent, uncle, grandparent).
    */
   opium_rbt_red(node);

   return node;
}

/* Rotations in RBT
 *
 * Rotations are the primary balancing tool in red-black trees.
 * 
 * Their purpose is to:
 *  - Preserve the order of keys in a BST
 *  - Correct violations of the RBT rules after allocation/deletion
 * 
 * There are two types of rotations:
 *  - Right rotation
 *  - Left rotation 
 */
   opium_rbt_node_t *
opium_rbt_right_rotate(opium_rbt_t *rbt, opium_rbt_node_t *y)
{
   opium_rbt_node_t *sentinel = rbt->sentinel;
   opium_rbt_node_t *x = y->left;
   if (x == sentinel) {
      return y;
   }

   /* Before rotation:
    *
    *     y
    *    / \
    *   x   C
    *  / \
    * A   B
    *
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

   /* After rotation:
    *
    *     x
    *    / \
    *   A   y
    *      / \
    *     B   C
    */

   return x;
}

   opium_rbt_node_t *
opium_rbt_left_rotate(opium_rbt_t *rbt, opium_rbt_node_t *y)
{
   opium_rbt_node_t *sentinel = rbt->sentinel;
   opium_rbt_node_t *x = y->right;
   if (x == sentinel) {
      return y;
   }

   /* Before rotation:
    *
    *    y
    *   / \
    *  A   x
    *     / \
    *    B   C
    *
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

   /* After rotation:
    *
    *        x
    *       / \
    *      y   C
    *     / \
    *    A   B
    *
    */

   return x;
}

   static opium_rbt_node_t *
opium_rbt_node_insert(opium_rbt_t *rbt, opium_rbt_node_t **root, opium_rbt_key_t key)
{
   /*
    * The purpose of this function is to find a location in the tree
    * where a new node with the key can be inserted.
    */

   opium_rbt_node_t *sentinel = rbt->sentinel;
   opium_rbt_node_t *parent = NULL;
   opium_rbt_node_t *current = *root;

   /*
    * The algorithm in a RBT (and in a BST in general) is always the same:
    *   - Start at the root (current = *root)
    *   Move downwards:
    *   - if current->key < key -> go right
    *   - if current->key > key -> go left
    *   - if key matches -> return NULL (do not insert duplicates)
    *
    *   When we reach sentinel, we have found a location where a new node can be inserted.
    */
   while (parent != sentinel) {
      parent = current;

      if (current->key < key) {
         current = current->right;
      } else if (current->key > key) {
         current = current->left;
      } else {
         return current;
      }

   }

   opium_rbt_node_t *node = opium_rbt_node_init(rbt, parent, key);
   if (!node) {
      return NULL;
   }

   /*
    * If the tree is empty, the new node becomes the root.
    * The root must be black -> recolor.
    */
   if (*root == sentinel) {
      node->parent = NULL;
      node->right = sentinel;
      node->left = sentinel;
      opium_rbt_black(node);
      *root = node;

      return node;
   }

   /* The new node is attached as a left or right child depending on the key comparison. */
   if (key < parent->key) {
      parent->left = node;
   } else {
      parent->right = node;
   }

   return node;
}

   opium_rbt_node_t *
opium_rbt_insert(opium_rbt_t *rbt, opium_rbt_key_t key, void *data)
{
   assert(rbt != NULL);

   opium_rbt_node_t **root = &rbt->head;

   opium_rbt_node_t *node = opium_rbt_node_insert(rbt, root, key);
   if (!node) {
      return NULL;
   }

   /* 
    * Insertion into a RBT works like this:
    * The new node is inserted as red and rule #4 may be violated.
    * To restore balance, we perform rotations and recolorings.
    * This is exactly what the while loop fixes.
    *
    * Prerequisites:
    * N is the new red node being inserted.
    * P is N`s parent.
    * U is P`s brother/sister/N`s uncle.
    * G is P`s parent.
    * 
    * Checking conditions:
    *  1) node != *root - until we reach the root.
    *  The root must always be black (rule #2)
    *  If we have reached the root, there is no point to checking the 'parent'
    *  further; it does not exist.
    *
    *  2) opium_rbt_is_red(parent) - is the parent red?
    *  If the parent is black - rule #4 is not violated, the balance is fine.
    *  If the parent is red - violation -> needs to be corrected.
    *
    * Conclusion: the cycle continues only if the insertion is violated rule #4
    * and we are not at the root.
    *
    */

   opium_rbt_node_t *temp;

   while (node != *root && opium_rbt_is_red(node->parent)) {
      if (node->parent->parent == NULL) break;
      /*
       * #1 - Determine Side
       *  We loot at whether the parent (P) is to the left of right of grandparent (G).
       *  This is important because the algorithm is symmetric:
       *   - If parent (P) is on the right - uncle (U) is on the left.
       *   - If parent (P) is on the left - uncle(U) is on the right.
       *
       * #2 - The "Uncle" is Red Case:
       *  We inserted a node (N) -> it is red.
       *  Its parent (P) is also red -> rule #4 is violated.
       *  Additionaly, G now has both children (P and U) are red.
       *  
       *  No rotations are needed here because the tree structure is
       *  symmetrical and the balance of the "black height" is maintained.
       *  Just need to repaint:
       *  P -> black.
       *  U -> black.
       *  G -> red.
       *
       * #3 - 
       * We have two options for the location of the node (N)
       * relative to the parent (P) and grandparent (G):
       *  - Linear cases (RR or LL)
       *  - Zigzag cases (RL or LR)
       * 
       * Linear cases: Here N and P are going in the same direction.
       * This is the most easily fixed with one big turn aroung G.
       *
       * Zigzag cases: 
       *
       */

      if (node->parent == node->parent->parent->right) {
         temp = node->parent->parent->left;        // # 1

         if (opium_rbt_is_red(temp)) {             // # 2
            /*
             *        G(B)
             *       /   \
             *    U(R)   P(R)
             *           /
             *         N(R)
             */

            opium_rbt_black(node->parent);
            opium_rbt_black(temp);
            opium_rbt_red(node->parent->parent);
            node = node->parent->parent;
         } else {                                  // # 3
            if (node == node->parent->left) {
               /* Right-Left case
                *    G
                *     \
                *      P
                *     /
                *    N
                */

               node = node->parent;
               opium_rbt_right_rotate(rbt, node);
            }

            /* Right-Right case
             *    G
             *     \
             *      P
             *       \
             *        N 
             */

            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_left_rotate(rbt, node->parent->parent);
         }

      } else {
         temp =node->parent->parent->right;        // # 1

         if (opium_rbt_is_red(temp)) {             // # 2
            /*
             *        G(B)
             *       /   \
             *    P(R)   U(R)
             *     \
             *      N(R)
             */

            opium_rbt_black(node->parent);
            opium_rbt_black(temp);
            opium_rbt_red(node->parent->parent);
            node = node->parent->parent;
         } else {                                  // # 3
            if (node == node->parent->right) {
               /* Left-Right case
                *      G
                *     /
                *    P
                *     \
                *      N
                */

               node = node->parent;
               opium_rbt_left_rotate(rbt, node);
            }

            /* Left-Left case
             *        G
             *       /
             *      P
             *     /
             *    N
             */

            opium_rbt_black(node->parent);
            opium_rbt_red(node->parent->parent);
            opium_rbt_right_rotate(rbt, node->parent->parent);
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
   assert(rbt != NULL);

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
      opium_slab_free(&rbt->slab, to_delete);
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
      opium_slab_free(&rbt->slab, to_delete);
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
            opium_rbt_left_rotate(rbt, parent);
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
               opium_rbt_right_rotate(rbt, sibling);
               sibling = parent->right;
            }
            /* Case 4: Sibling is black, right child is red */
            opium_rbt_copy_color(sibling, parent);
            opium_rbt_black(parent);
            opium_rbt_black(sibling->right);
            opium_rbt_left_rotate(rbt, parent);
            current = *root; /* Terminate loop */
         }
      } else {
         sibling = parent->left;

         if (opium_rbt_is_red(sibling)) {
            /* Case 1: Sibling is red (mirrored) */
            opium_rbt_red(parent);
            opium_rbt_black(sibling);
            opium_rbt_right_rotate(rbt, parent);
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
               opium_rbt_left_rotate(rbt, sibling);
               sibling = parent->left;
            }
            /* Case 4: Sibling is black, left child is red (mirrored) */
            opium_rbt_copy_color(sibling, parent);
            opium_rbt_black(parent);
            opium_rbt_black(sibling->left);
            opium_rbt_right_rotate(rbt, parent);
            current = *root; /* Terminate loop */
         }
      }
   }

   /* Ensure the current node is black */
   if (current != sentinel) {
      opium_rbt_black(current);
   }

   opium_rbt_black(*root); /* Ensure root is black */
   opium_slab_free(&rbt->slab, to_delete);
}

