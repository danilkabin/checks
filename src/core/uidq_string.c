#include "uidq_string.h"

size_t uidq_strlen(char *data, size_t size) {
   size_t index;
   for (index = 0; index < size; index++) {
      if (data[index] == '\0') {
         return index;
      }
   }
   return index;
}
