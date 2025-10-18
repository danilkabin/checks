/* opium_hash.c
 *
 * Hash Table Overview:
 *
 * This module implements a hash table with chaining for collision resolution,
 * optimized for string keys of fixed size. It uses two memory allocators:
 * - A pool allocator (`opium_pool_t`) for bucket heads, providing fast indexed access.
 * - A slab allocator (`opium_slab_t`) for collision elements, minimizing fragmentation.
 *
 * Key Features:
 * - Supports fixed-size string keys, normalized to ensure consistent hashing.
 * - Handles collisions via linked lists within each bucket, using `opium_listhead.h`.
 * - Provides robust error handling with logging for diagnostics.
 * - Ensures memory safety with proper allocation and cleanup.
 *
 * Memory Management:
 * - Buckets are stored in `hash->elts` (pool), where each bucket head is an
 *   `opium_hash_elt_t` containing a key, value, and linked list head for collisions.
 * - Collision elements are allocated from `hash->outcasts` (slab) and linked into
 *   the bucket’s list.
 * - Keys are normalized to `hash->key_size` bytes by padding with zeros if needed.
 *
 * Structure of `opium_hash_elt_t`:
 * - Header: `opium_hash_elt_t` (includes `opium_list_head_t head` for collisions).
 * - Key: `key_size` bytes, immediately following the header.
 * - Value: `value_size` bytes, following the key.
 *
 * Example Memory Layout (key_size=8, value_size=8):
 *   [opium_hash_elt_t | key (8 bytes) | value (8 bytes)]
 *   Total size = sizeof(opium_hash_elt_t) + 8 + 8
 *
 * Usage:
 * - Initialize with `opium_hash_init`, specifying bucket count, key size, and value size.
 * - Insert key-value pairs with `opium_hash_insert`.
 * - Find elements with `opium_hash_find`.
 * - Delete elements with `opium_hash_delete`.
 * - Clean up with `opium_hash_exit`.
 *
 * Alignment with opium_slab.c:
 * - Uses `opium_pool_t` and `opium_slab_t` for memory management.
 * - Employs detailed logging for errors and debugging.
 * - Includes comprehensive validation to prevent undefined behavior.
 */

#include "core/opium_core.h"
#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include "core/opium_log.h"
#include <stdio.h>
#include <string.h>

/* Minimum sizes and defaults */
#define OPIUM_HASHT_KEY_SIZE   8    /* Minimum key size in bytes */
#define OPIUM_HASHT_VALUE_SIZE 8    /* Minimum value size in bytes */
#define OPIUM_HASHT_CAPACITY   8    /* Default number of buckets */
#define OPIUM_HASHT_COLL_MULT  1    /* Default collision multiplier */

/* Check if hash table is valid */
   static inline int
opium_hash_isvalid(opium_hash_t *hash)
{
   /*
    * Validates the hash table by checking if it’s non-null and has valid fields.
    * Used to ensure the hash table is in a usable state before operations.
    */
   return hash && hash->nelts > 0 && hash->key_size >= OPIUM_HASHT_KEY_SIZE &&
      hash->value_size >= OPIUM_HASHT_VALUE_SIZE && hash->pair_size > 0;
}

/* Get key pointer for a hash element */
   opium_ubyte_t *
opium_hash_elt_key(opium_hash_elt_t *elt)
{
   /*
    * Returns a pointer to the key within a hash element.
    * The key starts immediately after the `opium_hash_elt_t` header.
    *
    * Returns NULL if the element is invalid to prevent crashes.
    */
   if (!elt) {
      return NULL;
   }
   return (opium_ubyte_t *)(elt + 1);
}

/* Get value pointer for a hash element */
   opium_ubyte_t *
opium_hash_elt_value(opium_hash_elt_t *elt, size_t key_size)
{
   /*
    * Returns a pointer to the value within a hash element.
    * The value starts after the key, which is `key_size` bytes after the header.
    *
    * Returns NULL if the element is invalid to prevent crashes.
    */
   if (!elt) {
      return NULL;
   }
   return opium_hash_elt_key(elt) + key_size;
}

/* Normalize string key by padding with zeros */
   static void
opium_hash_normalize_key(void *dst, void *src, size_t key_size)
{
   /*
    * Normalizes a string key to a fixed size by copying the key and padding
    * with zeros if the key is shorter than `key_size`. This ensures consistent
    * hashing for strings of varying lengths.
    *
    * Example:
    *   Input: key = "abc", key_size = 8
    *   Output: "abc\0\0\0\0\0"
    *
    * Edge Cases:
    * - If dst, src, or key_size is invalid, does nothing to avoid crashes.
    */
   if (!dst || !src || key_size == 0) {
      return;
   }
   size_t len = strnlen(src, key_size);
   memset(dst, 0, key_size);
   memcpy(dst, src, len < key_size ? len : key_size);
}

/* Calculate bucket index for a key */
   static size_t
opium_hash_elt_index(opium_hash_t *hash, void *key, opium_ubyte_t *normalized)
{
   /*
    * Computes the bucket index for a key by normalizing it and applying the
    * hash function. The index is modulo the number of buckets (`hash->nelts`).
    *
    * Steps:
    * 1. Validate inputs to prevent crashes.
    * 2. Normalize the key to `hash->key_size` bytes.
    * 3. Apply the hash function (e.g., `opium_hash_djb2`).
    * 4. Modulo the result by `hash->nelts` to get the bucket index.
    *
    * Returns 0 on invalid input, which callers must handle to avoid accessing
    * invalid buckets.
    */
   if (!opium_hash_isvalid(hash) || !key || !normalized) {
      return 0;
   }
   opium_hash_normalize_key(normalized, key, hash->key_size);
   return hash->func(normalized, hash->key_size) % hash->nelts;
}

/* Initialize hash table */
   int
opium_hash_init(opium_hash_t *hash, size_t nelts, size_t ks, size_t vs, opium_log_t *log)
{
   /*
    * Initializes a hash table with the specified number of buckets, key size,
    * and value size. Sets up the bucket pool and collision slab for efficient
    * memory management.
    *
    * Parameters:
    * - hash: The hash table structure to initialize.
    * - nelts: Number of buckets (must be positive).
    * - ks: Key size in bytes (minimum OPIUM_HASHT_KEY_SIZE).
    * - vs: Value size in bytes (minimum OPIUM_HASHT_VALUE_SIZE).
    * - log: Logging context for error and debug messages.
    *
    * Memory Allocation:
    * - Buckets: Stored in `hash->elts` (opium_pool_t) for fast indexed access.
    * - Collisions: Stored in `hash->outcasts` (opium_slab_t) for efficient allocation.
    * - Pair size: Calculated as sizeof(opium_hash_elt_t) + key_size + value_size.
    *
    * Steps:
    * 1. Validate input parameters.
    * 2. Set hash table fields (nelts, key_size, value_size, func).
    * 3. Initialize bucket pool and collision slab.
    * 4. Set logging context.
    *
    * Returns:
    * - OPIUM_RET_OK on success.
    * - OPIUM_RET_ERR on failure, with appropriate cleanup and logging.
    */
   if (!hash || nelts == 0 || ks < OPIUM_HASHT_KEY_SIZE || vs < OPIUM_HASHT_VALUE_SIZE) {
      if (log) {
         opium_log_err(log, "Invalid parameters for hash table initialization: "
               "nelts=%zu, key_size=%zu, value_size=%zu.\n", nelts, ks, vs);
      }
      return OPIUM_RET_ERR;
   }

   hash->nelts = nelts;
   hash->key_size = ks;
   hash->value_size = vs;
   hash->func = opium_hash_djb2;
   hash->pair_size = sizeof(opium_hash_elt_t) + ks + vs;
   hash->log = log;

   /* Initialize bucket pool */
   int ret = opium_pool_init(&hash->elts, hash->nelts, log);
   if (ret != OPIUM_RET_OK) {
      if (log) {
         opium_log_err(log, "Failed to initialize bucket pool with %zu buckets.\n", nelts);
      }
      return OPIUM_RET_ERR;
   }

   /* Initialize collision slab */
   ret = opium_slab_init(&hash->outcasts, hash->pair_size, log);
   if (ret != OPIUM_RET_OK) {
      if (log) {
         opium_log_err(log, "Failed to initialize collision slab for pair_size %zu.\n", hash->pair_size);
      }
      opium_pool_exit(&hash->elts);
      return OPIUM_RET_ERR;
   }

   if (log) {
      opium_log_debug(log, "Hash table initialized: nelts=%zu, key_size=%zu, value_size=%zu, pair_size=%zu.\n",
            hash->nelts, hash->key_size, hash->value_size, hash->pair_size);
   }

   return OPIUM_RET_OK;
}

/* Clean up hash table */
   void
opium_hash_exit(opium_hash_t *hash)
{
   /*
    * Frees all resources associated with the hash table and resets its state
    * to prevent accidental reuse or dangling pointers.
    *
    * Steps:
    * 1. Free the collision slab (`hash->outcasts`).
    * 2. Free the bucket pool (`hash->elts`).
    * 3. Reset all fields to zero or NULL.
    *
    * Edge Cases:
    * - Checks if the slab and pool are initialized before freeing.
    * - Safe to call on an invalid or partially initialized hash table.
    */
   if (!hash) {
      return;
   }

   opium_slab_exit(&hash->outcasts);
   opium_pool_exit(&hash->elts);

   hash->nelts = 0;
   hash->key_size = 0;
   hash->value_size = 0;
   hash->pair_size = 0;
   hash->func = NULL;
   hash->log = NULL;

   opium_log_debug(hash->log, "Hash table destroyed.\n");
}

/* Insert a key-value pair into the hash table */
   int
opium_hash_insert(opium_hash_t *hash, void *key, void *value)
{
   /*
    * Inserts a key-value pair into the hash table. If the key exists, updates
    * the value. Otherwise, creates a new element in the bucket pool (if empty)
    * or collision slab (if collision).
    *
    * Steps:
    * 1. Validate inputs and hash table state.
    * 2. Normalize the key and compute its bucket index.
    * 3. Check if the key exists; if so, update the value.
    * 4. If the bucket is empty, allocate in the pool and initialize the list head.
    * 5. If the bucket has elements, allocate in the slab and append to the collision list.
    * 6. Copy normalized key and value into the new element.
    *
    * Memory Management:
    * - Bucket heads are allocated in `hash->elts` (pool).
    * - Collision elements are allocated in `hash->outcasts` (slab).
    * - Keys are normalized to ensure consistent hashing.
    *
    * Returns:
    * - OPIUM_RET_OK on success.
    * - OPIUM_RET_ERR on failure (e.g., allocation failure), with logging.
    */
   if (!opium_hash_isvalid(hash) || !key) {
      return OPIUM_RET_ERR;
   }

   /* Normalize key and compute index */
   opium_ubyte_t normalized[hash->key_size];
   size_t index = opium_hash_elt_index(hash, key, normalized);
   if (index >= hash->nelts) {
      opium_log_err(hash->log, "Computed index %zu exceeds bucket count %zu.\n", index, hash->nelts);
      return OPIUM_RET_ERR;
   }

   /* Check if key exists */
   opium_hash_elt_t *elt = opium_hash_find(hash, key);
   if (elt) {
      if (value) {
         opium_memcpy(opium_hash_elt_value(elt, hash->key_size), value, hash->value_size);
         opium_log_debug(hash->log, "Updated value for key at index %zu.\n", index);
      }
      return OPIUM_RET_OK;
   }

   /* Get bucket head */
   opium_hash_elt_t *authority = opium_pool_get(&hash->elts, index);

   /* Allocate new element */
   if (!authority) {
      /* Bucket is empty, allocate in pool */
      elt = opium_pool_push(&hash->elts, hash->pair_size, index);
      if (!elt) {
         opium_log_err(hash->log, "Failed to allocate bucket element at index %zu.\n", index);
         return OPIUM_RET_ERR;
      }
      OPIUM_INIT_LIST_HEAD(&elt->head);
   } else {
      /* Bucket has elements, allocate in slab */
      elt = opium_slab_alloc(&hash->outcasts);
      if (!elt) {
         opium_log_err(hash->log, "Failed to allocate collision element for index %zu.\n", index);
         return OPIUM_RET_ERR;
      }
      opium_list_add_tail(&elt->head, &authority->head);
   }

   /* Copy normalized key and value */
   opium_memcpy(opium_hash_elt_key(elt), normalized, hash->key_size);
   if (value) {
      opium_memcpy(opium_hash_elt_value(elt, hash->key_size), value, hash->value_size);
   } else {
      opium_memzero(opium_hash_elt_value(elt, hash->key_size), hash->value_size);
   }

   opium_log_debug(hash->log, "Inserted new key-value pair at index %zu.\n", index);
   return OPIUM_RET_OK;
}

/* Delete a key-value pair from the hash table */
   void
opium_hash_delete(opium_hash_t *hash, void *key)
{
   if (!hash || !key) {
      if (hash && hash->log) {
         opium_log_err(hash->log, "Invalid hash or key for delete.\n");
      }
      return;
   }

   if (!opium_hash_isvalid(hash)) {
      opium_log_err(hash->log, "Invalid hash table state for delete.\n");
      return;
   }

   opium_ubyte_t normalized[hash->key_size];
   size_t index = opium_hash_elt_index(hash, key, normalized);
   if (index >= hash->nelts) {
      opium_log_err(hash->log, "Computed index %zu exceeds bucket count %zu.\n", index, hash->nelts);
      return;
   }

   opium_hash_elt_t *authority = opium_pool_get(&hash->elts, index);
   if (!authority) {
      return;
   }

   if (!opium_list_is_linked(&authority->head)) {
      opium_log_err(hash->log, "Bucket head at index %zu is unlinked.\n", index);
      return;
   }

   /* Check if bucket head matches */
   if (opium_memcmp(normalized, opium_hash_elt_key(authority), hash->key_size) == 0) {
      if (!opium_list_empty(&authority->head)) {
         /* Promote first collision element to bucket head */
         opium_hash_elt_t *next = opium_list_first_entry(&authority->head, opium_hash_elt_t, head);
         if (next && opium_list_is_linked(&next->head)) {
            opium_memcpy(opium_hash_elt_key(authority), opium_hash_elt_key(next), hash->key_size);
            opium_memcpy(opium_hash_elt_value(authority, hash->key_size),
                  opium_hash_elt_value(next, hash->key_size), hash->value_size);
            opium_list_del(&next->head);
            OPIUM_INIT_LIST_HEAD(&next->head); /* Ensure freed element is unlinked */
            opium_log_debug(hash->log, "Freeing collision element at index %zu, ptr=%p.\n", index, next);
            opium_slab_free(&hash->outcasts, next);
            opium_log_debug(hash->log, "Deleted bucket head at index %zu, promoted collision element.\n", index);
         } else {
            opium_log_err(hash->log, "Invalid collision element at index %zu.\n", index);
         }
      } else {
         /* No collisions, remove bucket head */
         opium_log_debug(hash->log, "Freeing bucket head at index %zu, ptr=%p.\n", index, authority);
         opium_pool_pop(&hash->elts, index);
         opium_log_debug(hash->log, "Deleted bucket head at index %zu.\n", index);
      }
      return;
   }

   /* Search collision list */
   opium_hash_elt_t *current, *tmp;
   opium_list_for_each_entry_safe(current, tmp, &authority->head, head) {
      if (!opium_list_is_linked(&current->head)) {
         opium_log_err(hash->log, "Encountered unlinked collision element at index %zu.\n", index);
         continue;
      }
      if (opium_memcmp(normalized, opium_hash_elt_key(current), hash->key_size) == 0) {
         opium_list_del(&current->head);
         OPIUM_INIT_LIST_HEAD(&current->head); /* Ensure freed element is unlinked */
         opium_log_debug(hash->log, "Freeing collision element at index %zu, ptr=%p.\n", index, current);
         opium_slab_free(&hash->outcasts, current);
         opium_log_debug(hash->log, "Deleted collision element at index %zu.\n", index);
         return;
      }
   }
}

/* Find an element by key in the hash table */
   opium_hash_elt_t *
opium_hash_find(opium_hash_t *hash, void *key)
{
   /*
    * Finds an element in the hash table by its key.
    *
    * Steps:
    * 1. Validate inputs and compute bucket index using normalized key.
    * 2. Check the bucket head for a match.
    * 3. If no match, iterate over the collision list to find the key.
    *
    * Returns:
    * - Pointer to the found element (`opium_hash_elt_t *`).
    * - NULL if the key is not found or inputs are invalid.
    *
    * Notes:
    * - Uses normalized key for consistent comparison.
    * - Removed unsafe debug printf to avoid assuming string keys.
    * - Uses safe list iteration to prevent issues with list modifications.
    */
   if (!opium_hash_isvalid(hash) || !key) {
      return NULL;
   }

   /* Normalize key and compute index */
   opium_ubyte_t normalized[hash->key_size];
   size_t index = opium_hash_elt_index(hash, key, normalized);
   if (index >= hash->nelts) {
      opium_log_err(hash->log, "Computed index %zu exceeds bucket count %zu.\n", index, hash->nelts);
      return NULL;
   }

   /* Get bucket head */
   opium_hash_elt_t *authority = opium_pool_get(&hash->elts, index);
   if (!authority) {
      return NULL;
   }

   /* Check bucket head */
   if (opium_memcmp(normalized, opium_hash_elt_key(authority), hash->key_size) == 0) {
      opium_log_debug(hash->log, "Found key in bucket head at index %zu.\n", index);
      return authority;
   }

   /* Search collision list */
   opium_hash_elt_t *current, *tmp;
   opium_list_for_each_entry_safe(current, tmp, &authority->head, head) {
      if (opium_memcmp(normalized, opium_hash_elt_key(current), hash->key_size) == 0) {
         opium_log_debug(hash->log, "Found key in collision list at index %zu.\n", index);
         return current;
      }
   }

   return NULL;
}

/* Debug hash table contents */
   void
opium_hash_debug(opium_hash_t *hash)
{
   /*
    * Prints the contents of the hash table for debugging purposes.
    * Displays each bucket and its collision list, showing keys and values.
    *
    * Notes:
    * - Assumes keys and values are printable strings for simplicity.
    * - In a production environment, this should be modified to handle binary data.
    * - Styled like `opium_slab_stats` for consistent debugging output.
    *
    * Output Format:
    * - Lists each bucket by index.
    * - Shows the key and value for the bucket head.
    * - Shows keys and values for all collision elements.
    */
   if (!opium_hash_isvalid(hash)) {
      return;
   }

   opium_log_debug(hash->log, "=== HASH TABLE DEBUG ===\n");

   for (size_t index = 0; index < hash->nelts; ++index) {
      opium_hash_elt_t *elt = opium_pool_get(&hash->elts, index);
      if (!elt) {
         continue;
      }

      opium_log_debug_inline(hash->log, "Bucket %zu:\n", index);
      /* Print bucket head */
      opium_log_debug_inline(hash->log, "  Key: '%.*s', Value: '%.*s'\n",
            (int)hash->key_size, opium_hash_elt_key(elt),
            (int)hash->value_size, opium_hash_elt_value(elt, hash->key_size));

      /* Print collision list */
      opium_hash_elt_t *current, *tmp;
      opium_list_for_each_entry_safe(current, tmp, &elt->head, head) {
         opium_log_debug_inline(hash->log, "  Key: '%.*s', Value: '%.*s'\n",
               (int)hash->key_size, opium_hash_elt_key(current),
               (int)hash->value_size, opium_hash_elt_value(current, hash->key_size));
      }
   }

   opium_log_debug_inline(hash->log, "\n");
}
