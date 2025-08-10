#ifndef UIDQ_SLAB_H
#define UIDQ_SLAB_H

#include "core/uidq_log.h"
#include "uidq_bitmask.h"

typedef enum {
   UIDQ_SLAB_BUSY,
} uidq_slab_ret_t;

typedef struct {
   size_t count;
   void *data;
   int index;
} uidq_slab_block_t;

// Structure representing a slab allocator
typedef struct {
    bool initialized;          // Indicates if the slab is initialized (true = initialized)
    size_t size;               // Total size of the slab in bytes
    size_t block_count;        // Total number of blocks in the slab
    size_t block_size;         // Size of each block in bytes
    size_t block_busy;         // Number of allocated (busy) blocks
    size_t block_free;         // Number of free blocks
    int last_free_pos; 
    void *data;                // Pointer to the slab's data buffer
    uidq_slab_block_t *block_data; // Array of metadata for each block
    uidq_bitmask_t *bitmask;   // Pointer to bitmask tracking block allocation
    uidq_log_t *log;           // Pointer to the logging structure
} uidq_slab_t;

// Slab creation and initialization
uidq_slab_t *uidq_slab_create(void);
int uidq_slab_init(uidq_slab_t *slab, uidq_log_t *log, size_t count, size_t block_size);
uidq_slab_t *uidq_slab_create_and_init(uidq_log_t *log, size_t count, size_t block_size);
int uidq_slab_reinit(uidq_slab_t **pslab, size_t count);

// Block management
void uidq_slab_block_init(uidq_slab_block_t *block, size_t count, void *data, int index);
void uidq_slab_block_exit(uidq_slab_block_t *block);
int uidq_slab_blocks_init(uidq_slab_t *slab, size_t count);

uidq_slab_block_t *uidq_slab_push(uidq_slab_t **pslab, void *data, size_t size);
uidq_slab_block_t *uidq_slab_pushin(uidq_slab_t **pslab, void *data, size_t size);
uidq_slab_block_t *uidq_slab_block_copy(uidq_slab_t *src, int src_pos, uidq_slab_t *dst, int dst_pos);

int uidq_block_realloc(uidq_slab_t *slab, int pos, void *new_data, size_t new_size);

// Information retrieval
bool uidq_slab_is_valid(const uidq_slab_t *slab, int pos, size_t size);
bool uidq_slab_is_empty(const uidq_slab_t *slab);
uidq_slab_block_t *uidq_slab_get_block(const uidq_slab_t *slab, int pos);
void *uidq_slab_get(uidq_slab_t *slab, int pos);
int uidq_slab_get_block_free(const uidq_slab_t *slab);
int uidq_slab_get_block_busy(const uidq_slab_t *slab);
int uidq_slab_is_block_allocated(const uidq_slab_t *slab, int pos);
void uidq_slab_update_counts(uidq_slab_t *slab, int64_t busy_delta, int64_t free_delta);
bool uidq_slab_block_is_valid(const uidq_slab_block_t *block);
void uidq_slab_info(uidq_slab_t *slab);
void uidq_slab_info_block(uidq_slab_t *slab, int pos);
void uidq_slab_reset(uidq_slab_t *slab);

// Deallocation
void uidq_slab_pop(uidq_slab_t *slab, int pos);

void uidq_slab_exit(uidq_slab_t *slab);
void uidq_slab_free(uidq_slab_t *slab);

#endif
