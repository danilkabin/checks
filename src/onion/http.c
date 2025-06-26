#include "http.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

struct onion_http_choice ONION_HTTP_METHOD_LIST[] = {
  {"GET", ONION_HTTP_METHOD_GET},
  {"POST", ONION_HTTP_METHOD_POST},
  {"PUT", ONION_HTTP_METHOD_PUT},
  {"DELETE", ONION_HTTP_METHOD_DELETE},
  {"HEAD", ONION_HTTP_METHOD_HEAD},
  {"OPTIONS", ONION_HTTP_METHOD_OPTIONS},
  {"PATCH", ONION_HTTP_METHOD_PATCH},
};

struct onion_http_choice ONION_HTTP_VERSION_LIST[] = {
  {"HTTP/1.1", ONION_HTTP_VERSION_1_1},
};

char *onion_chacha(char *start, size_t size, char target) {
  for (size_t offset = 0; offset < size; offset++) {
    if (start[offset] == target) {
      return &start[offset];
    }
  }
  return NULL;
}

char *onion_stringa(char *start, size_t size, char *target, size_t target_size) {
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

int onion_http_choice_get(struct onion_http_choice *choice, size_t choice_count, const char *name, size_t size) {
   for (size_t index = 0; index < choice_count; index++) {
      struct onion_http_choice target = choice[index];
      size_t method_len = strlen(target.name);
      if (method_len == size && strncmp(name, target.name, size) == 0) {
         return target.token;
      }
   }
   return -1;
}

int onion_http_get_valid_method(const char *name, size_t size) {
   size_t choice_count = sizeof(ONION_HTTP_METHOD_LIST) / sizeof(ONION_HTTP_METHOD_LIST[0]);
   int ret = onion_http_choice_get(ONION_HTTP_METHOD_LIST, choice_count, name, size);
   return ret >= 0 ? ret : ONION_HTTP_METHOD_INVALID;
}

int onion_http_get_valid_version(const char *name, size_t size) {
   size_t choice_count = sizeof(ONION_HTTP_VERSION_LIST) / sizeof(ONION_HTTP_VERSION_LIST[0]);
   int ret = onion_http_choice_get(ONION_HTTP_VERSION_LIST, choice_count, name, size);
   return ret >= 0 ? ret : ONION_HTTP_VERSION_INVALID;
}

int onion_http_check_in_range(const char *data, size_t size, unsigned char min, unsigned char max, unsigned char *exp, size_t size_exp) {
  for (size_t index = 0; index < size; index++) {
    unsigned char c = (unsigned char)data[index];

    if (c >= min && c <= max) {
      continue;
    }
    int allowed = 0;
    if (exp && size_exp > 0) {
      for (size_t i2 = 0; i2 < size_exp; i2++) {
        if (c == exp[i2]) {
          allowed = 1;
          break;
        }
      }
    }
    if (!allowed) {
      return -1;
    }
  }
  return 0;
}

int onion_http_get_valid_header_name(char *data, size_t size) {

   if (onion_chacha(data, size, '\0') != NULL) {
      return -1;
   }

  for (size_t index = 0; index < size; index++) {
    unsigned char c = (unsigned char)data[index];
    if (c < 33 || c > 126) {
      return -1;
    }

    if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
      c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
      c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
      c == '{' || c == '}' || c == ' ' || c == '\t')
      return -1;

  }
  return 0;
}

int onion_http_buff_append(onion_http_buffer_t *buff, char *data, size_t len) {
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

    char *new_data = onion_slab_realloc(buff->allocator, buff->data, new_capacity);
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

int onion_http_buff_data_len(onion_http_buffer_t *buff) {
  return buff->end - buff->start;
}

char *onion_http_buff_data_start(onion_http_buffer_t *buff) {
  return buff->data + buff->start;
}

void onion_http_buff_consume(onion_http_buffer_t *buff, size_t len) {
  buff->start = buff->start + len;
  if (buff->start >= buff->end) {
    buff->start = 0;
    buff->end = 0;
  }
}

int onion_http_buff_init(struct onion_slab *allocator, onion_http_buffer_t *buff, size_t capacity, size_t limit) {
  if (limit < 8) {
    return -1;
  }
  buff->data = onion_slab_malloc(allocator, NULL, capacity);
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

int onion_http_buff_reinit(struct onion_slab *allocator, onion_http_buffer_t *buff, size_t capacity, size_t limit) {
  onion_http_buff_exit(buff);
  return onion_http_buff_init(allocator, buff, capacity, limit);
}

void onion_http_buff_exit(onion_http_buffer_t *buff) {
  if (buff->data) {
    onion_slab_free(buff->allocator, buff->data);
  }
  buff->start = 0;
  buff->end = 0;
  buff->capacity = 0;
  buff->limit = 0;
  buff->allocator = NULL;
}

struct onion_slab *onion_slab_memory_init(size_t size) {
  if (size < 8) {
    return NULL;;
  }
  return onion_slab_init(size);
}

void onion_slab_memory_exit(struct onion_slab *allocator) {
  onion_slab_exit(allocator);
}
