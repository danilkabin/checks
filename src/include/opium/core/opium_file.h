#ifndef OPIUM_FILE_INCLUDE_H
#define OPIUM_FILE_INCLUDE_H 

#include "core/opium_core.h"

#define OPIUM_MAX_PATH_LEVEL    3

struct opium_file_s {
   opium_fd_t     fd;
   opium_string_t name;

   /* struct stat */
   size_t         size;
   opium_time_t   mtime;
   opium_mode_t   mode;

   unsigned       is_dir:1;
   unsigned       readonly:1;
   unsigned       executable:1;
   unsigned       reserved:5;

   /* data */
   u_char        *start, *end;
   u_char        *pos;
   ssize_t        len;

   opium_arena_t *arena;

   opium_log_t   *log;
};

struct opium_path_s {
   opium_string_t name;
   opium_uint_t   access;

   size_t         level[OPIUM_MAX_PATH_LEVEL];
};

size_t       opium_file_size(opium_file_t *file);
opium_time_t opium_file_mtime(opium_file_t *file);
opium_mode_t opium_file_mode(opium_file_t *file);
opium_int_t  opium_file_is_dir(opium_file_t *file);
opium_int_t  opium_file_is_readonly(opium_file_t *file);
opium_int_t  opium_file_is_executable(opium_file_t *file);

opium_fd_t   opium_file_open(opium_file_t *file, char *name, opium_arena_t *arena, opium_int_t flags, opium_log_t *log);
void         opium_file_close(opium_file_t *file);

ssize_t      opium_file_read(opium_file_t *file, void *buff, size_t len);
ssize_t      opium_file_write(opium_file_t *file, void *buff, size_t len);
opium_int_t  opium_file_load(opium_file_t *file, size_t len);
ssize_t      opium_file_read_all(opium_file_t *file, void *buff);

opium_uint_t opium_create_optimized_path(opium_path_t *path, opium_file_t *file);
opium_uint_t opium_create_full_path(u_char *dir, opium_uint_t access);

#endif /* OPIUM_FILE_INCLUDE_H  */

