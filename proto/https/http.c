#include "http.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

struct http_choice HTTP_METHOD_LIST[] = {
   {"GET", HTTP_METHOD_GET},
   {"POST", HTTP_METHOD_POST},
   {"PUT", HTTP_METHOD_PUT},
   {"DELETE", HTTP_METHOD_DELETE},
   {"HEAD", HTTP_METHOD_HEAD},
   {"OPTIONS", HTTP_METHOD_OPTIONS},
   {"PATCH", HTTP_METHOD_PATCH},
};

struct http_choice HTTP_VERSION_LIST[] = {
   {"HTTP/1.1", HTTP_VERSION_1_1},
};

char *http_findChar(char *start, size_t size, char target) {
   for (size_t offset = 0; offset < size; offset++) {
      if (start[offset] == target) {
         return &start[offset];
      }
   }
   return NULL;
}

char *http_findStr(char *start, size_t size, char *target, size_t target_size) {
   if (target_size <= 0 || size < target_size) {
      return NULL;
   }
   for (size_t offset = 0; offset <= size - target_size; offset++) {
      if (memcmp(&start[offset], target, target_size) == 0) {
         return &start[offset];
      }
   }
   return NULL;
}

int http_check_header(const char *name, const char *target) {
   return strcasecmp(name, target) == 0 ? 1 : -1;
}

int http_choice_get(struct http_choice *choice, size_t choice_count, const char *name, size_t size) {
   for (size_t index = 0; index < choice_count; index++) {
      struct http_choice target = choice[index];
      size_t method_len = strlen(target.name);
      if (method_len == size && strncmp(name, target.name, size) == 0) {
         return target.token;
      }
   }
   return -1; 
}

int http_get_valid_method(const char *name, size_t size) {
   size_t choice_count = sizeof(HTTP_METHOD_LIST) / sizeof(HTTP_METHOD_LIST[0]);
   int ret = http_choice_get(HTTP_METHOD_LIST, choice_count, name, size);
   return ret >= 0 ? ret : HTTP_METHOD_INVALID;
}

int http_get_valid_version(const char *name, size_t size) {
   size_t choice_count = sizeof(HTTP_VERSION_LIST) / sizeof(HTTP_VERSION_LIST[0]);
   int ret = http_choice_get(HTTP_VERSION_LIST, choice_count, name, size);
   return ret >= 0 ? ret : HTTP_VERSION_INVALID;
}

int http_buff_append(http_buffer_t *buff, char *data, size_t len) {
   size_t available = buff->capacity - buff->end;
   size_t used = buff->end - buff->start;

   if (available < len) {
      if (buff->start > 0) {
         memmove(buff->data, buff->data + buff->start, used);
         buff->start = 0;
         buff->end = used;
         if (buff->capacity - buff->end > len) {
            goto copy_data;
         }
      }

      size_t required = buff->end + len;
      size_t new_capacity = buff->capacity;
      while (new_capacity < required) {
         new_capacity = new_capacity * 2;
         if (new_capacity > buff->limit) {
            return -1;
         } 
      }

      char *new_data = slab_realloc(buff->allocator, buff->data, new_capacity);
      if (!new_data) {
         return -1;
      }
      buff->data = new_data;
      buff->capacity = new_capacity;
   }

copy_data:
   memcpy(buff->data + buff->end, data, len);
   buff->end = buff->end + len;

   return 0;
}

int http_buff_data_len(http_buffer_t *buff) {
   return buff->end - buff->start;
}

char *http_buff_data_start(http_buffer_t *buff) {
   return buff->data + buff->start;
}

void http_buff_consume(http_buffer_t *buff, size_t len) {
   buff->start = buff->start + len;
   if (buff->start >= buff->end) {
      buff->start = 0;
      buff->end = 0;
   }
}

int http_buff_init(struct slab *allocator, http_buffer_t *buff, size_t capacity, size_t limit) {
   if (limit < 8) {
      return -1;
   }
   buff->data = slab_malloc(allocator, NULL, capacity);
   if (!buff->data) {
      return -1;
   }
   buff->start = 0;
   buff->end = 0;
   buff->capacity = capacity;
   buff->limit = limit;
   buff->allocator = allocator;
   return 0;
}

int http_buff_reinit(struct slab *allocator, http_buffer_t *buff, size_t capacity, size_t limit) {
   http_buff_exit(buff);
   return http_buff_init(allocator, buff, capacity, limit);
}

void http_buff_exit(http_buffer_t *buff) {
   if (buff->data) {
      slab_free(buff->allocator, buff->data);
   }
   buff->start = 0;
   buff->end = 0;
   buff->capacity = 0;
   buff->limit = 0;
   buff->allocator = NULL;
}

struct slab *http_slab_memory_init(size_t size) {
   if (size < 8) {
      return NULL;;
   }
   return slab_init(size); 
}

void http_slab_memory_exit(struct slab *allocator) {
   slab_exit(allocator);
}
