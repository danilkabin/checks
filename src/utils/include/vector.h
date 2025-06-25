#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef struct {
   void **items;
   size_t count;
   size_t capacity;
} index_table;

void indextable_set(index_table *table, void *data, size_t index);
void *indextable_get(index_table *table, size_t index);

void indextable_init(index_table *table, size_t capacity);
void indextable_exit(index_table *table);

#endif
