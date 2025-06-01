#include "utils.h"
#include "vector.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void indextable_set(index_table *table, void *data, size_t index) {
   if (!table || !table->items) {
      DEBUG_FUNC("table is null!\n");
      return;
   }
   if (index >= table->capacity) {
      DEBUG_FUNC("index >= table capacity\n");
      return;
   }
   table->count += (table->items[index] == NULL && data != NULL) ? 1 : (table->items[index] != NULL && data == NULL) ? -1 : 0;
   table->items[index] = data;
}

void *indextable_get(index_table *table, size_t index) {
   if (!table || !table->items) {
      DEBUG_FUNC("table is null!\n");
      return NULL;
   }
   
   if (index >= table->capacity) {
      DEBUG_FUNC("index >= table capacity\n");
      return NULL;
   }
   return table->items[index];
}

void indextable_init(index_table *table, size_t capacity) {
   size_t size = sizeof(void*) * capacity;
   table->items = malloc(size);
   if (!table->items) {
      DEBUG_FUNC("items wasn`t inited\n");
      table->capacity = 0;
      table->count = 0;
      return;
   }
   table->capacity = capacity;
   table->count = 0;
   memset(table->items, 0, size); 
}

void indextable_exit(index_table *table) {
   if (table->items) {
      free(table->items);
   }
   table->items = NULL;
   table->capacity = 0;
   table->count = 0;
}
