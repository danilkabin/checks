#ifndef UIDQ_STRING_H
#define UIDQ_STRING_H

#include <stddef.h>

typedef struct {
   char *data;
   size_t size;
} uidq_string_t;

size_t uidq_strlen(char *data, size_t size);

#endif
