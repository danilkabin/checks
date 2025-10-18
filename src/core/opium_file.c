/* opium_file.c
 *
 * File Handling Overview:
 *
 * This module provides abstractions for file operations including:
 *  - Opening and closing files safely
 *  - Reading/writing with interruption handling
 *  - File metadata retrieval (size, mode, timestamps)
 *  - Creating directories in a structured way
 *
 * The purpose is to provide a robust and uniform interface for file
 * operations with built-in logging, memory management (via arenas), 
 * and safety against partial reads/writes.
 */

#include "core/opium_core.h"

#define OPIUM_FILE_BUFF_SIZE 512

/* Typedef for a generic read/write function pointer.
 * This allows opium_file_safe_rw to use either read() or write()
 * transparently while handling interruptions.
 */
typedef ssize_t (*opium_rw_func_t)(int fd, void *buff, size_t len); 

   ssize_t
opium_file_safe_rw(opium_fd_t fd, void *buff, size_t len, opium_rw_func_t func, opium_log_t *log)
{
   /* 
    * Why a while loop is used here instead of just one call
    *
    * In POSIX systems, functions like read() and write() do not guarantee
    * that they will process the entire buffer in one call.
    *
    * Even if you ask to read or write len bytes, 
    * the system might only handle a partial amount.
    * This is why a loop is necessary: 
    * to continue processing the remaining bytes until 
    * the full buffer is done or EOF/error occu* rs.
    */
   ssize_t pastebin;
   size_t  total = 0;

   while (total < len) {
      pastebin = func(fd, (u_char*)buff + total, len - total);
      if (pastebin < 0) {
         /* Interrupt */
         if (errno == EINTR) {
            continue;
         }

         opium_log_err(log, "Failed to load file!\n");
         return OPIUM_RET_ERR;
      }

      if (pastebin == 0) {
         break;
      }

      total = total + pastebin; 
   }

   return total;
}

   static void
opium_file_reset(opium_file_t *file)
{
   file->size = file->mtime = file->mode = file->is_dir = file->readonly = file->executable = 0;
}

   static void
opium_file_update(opium_file_t *file)
{
   /*
    * Purpose: Populate opium_file_t fields with actual filesystem info.
    * Calls fstat(fd, &stats) to get info about the file descriptor.
    *
    * If successful:
    * size       -> file size in bytes.
    * mtime      -> last modification timestamp.
    * mode       -> permission bits and type.
    * is_dir     -> whether it’s a directory (S_ISDIR macro).
    * readonly   -> true if user cannot write.
    * executable -> true if user can execute.
    *
    * If fstat fails, reset the struct with opium_file_reset.
    */
   struct stat stats;
   if (fstat(file->fd, &stats) == 0) {
      file->size       = stats.st_size;
      file->mtime      = stats.st_mtime;
      file->mode       = stats.st_mode;
      file->is_dir     = S_ISDIR(stats.st_mode);
      file->readonly   = !(stats.st_mode & S_IWUSR);
      file->executable = (file->mode & S_IXUSR); 
   } else {
      opium_file_reset(file);
   }
}

   size_t
opium_file_size(opium_file_t *file)
{
   assert(file != NULL);
   return file->size;
}

   opium_time_t
opium_file_mtime(opium_file_t *file)
{
   assert(file != NULL);
   return file->mtime;
}

   opium_mode_t
opium_file_mode(opium_file_t *file)
{
   assert(file != NULL);
   return file->mode;
}

   opium_int_t
opium_file_is_dir(opium_file_t *file)
{
   assert(file != NULL);
   return file->is_dir;
}

   opium_int_t
opium_file_is_readonly(opium_file_t *file)
{
   assert(file != NULL);
   return file->readonly;
}

   opium_int_t 
opium_file_is_executable(opium_file_t *file) 
{
   assert(file != NULL);
   return file->executable;
}

   opium_fd_t
opium_file_open(opium_file_t *file, char *name, opium_arena_t *arena, opium_int_t flags, opium_log_t *log)
{
   assert(file != NULL);
   assert(name != NULL);

   opium_int_t result;

   file->fd = open(name, flags, (flags & O_CREAT) ? 0644 : 0);
   if (file->fd < 0) {
      return OPIUM_RET_ERR;
   }

   opium_file_update(file);
   opium_string_set(&file->name, name);

   file->start = opium_arena_alloc(arena, file->size);
   if (!file->start) {
      opium_log_err(log, "Failed to allocate pos: %s\n", name);
      goto fail;
   } 

   result = opium_file_load(file, file->size);
   if (result != OPIUM_RET_OK) {
      goto fail;
   }

   file->end   = file->start + file->len;
   file->pos   = file->start;
   file->arena = arena;
   file->log   = log;

   return file->fd;

fail:
   opium_file_close(file);
   return OPIUM_RET_ERR;
}

   void
opium_file_close(opium_file_t *file)
{
   assert(file != NULL);

   if (file->fd) {
      close(file->fd);
      file->fd = -1;
   }

   if (file->start) {
      opium_arena_free(file->arena, file->start);
      file->start = NULL;
   }

   file->len = 0;

   opium_string_null(&file->name);
   opium_file_reset(file);

   file->end   = NULL;
   file->pos   = NULL;
   file->arena = NULL;
   file->log   = NULL;
}

   ssize_t
opium_file_read(opium_file_t *file, void *buff, size_t len)
{
   assert(file != NULL);
   assert(buff != NULL);
   assert(len > 0 || len >= (size_t)file->len);

   u_char *new_pos = file->pos + len;

   if (new_pos >= file->end) {
      return OPIUM_RET_FULL;
   }

   return opium_file_safe_rw(file->fd, buff, len, (opium_rw_func_t)read, file->log);
}

   ssize_t
opium_file_write(opium_file_t *file, void *buff, size_t len)
{
   assert(file != NULL);
   assert(buff != NULL);
   assert(len > 0 || len >= (size_t)file->len);

   u_char *new_pos = file->pos + len;

   if (new_pos >= file->end) {
      return OPIUM_RET_FULL;
   }

   return opium_file_safe_rw(file->fd, buff, len, (opium_rw_func_t)write, file->log);
}

   opium_int_t
opium_file_load(opium_file_t *file, size_t len)
{
   file->len = opium_file_safe_rw(file->fd, file->start, file->size, (opium_rw_func_t)read, file->log);
   return file->len >= 0 ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   ssize_t
opium_file_read_all(opium_file_t *file, void *buff)
{
   assert(file != NULL);
   assert(buff != NULL);

   file->pos = file->start;
   opium_memcpy(buff, file->start, file->len);

   return file->len;
}

   opium_int_t
opium_file_chmod(opium_file_t *file, opium_mode_t mode)
{
   assert(file != NULL);
   return fchmod(file->fd, mode) == 0 ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   opium_int_t
opium_file_chown(opium_file_t *file, opium_uid_t uid, opium_gid_t gid)
{
   assert(file != NULL);
   return fchown(file->fd, uid, gid) == 0 ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   opium_int_t
opium_file_faccessat(opium_file_t *file, opium_fd_t fd, opium_int_t type, opium_int_t flag)
{
   assert(file != NULL);
   return (faccessat(fd, (char*)file->name.data, type, flag) == 0) ? OPIUM_RET_OK : OPIUM_RET_ERR;
}

   opium_uint_t
opium_create_optimized_path(opium_path_t *path, opium_file_t *file)
{
   /*
    * This function creates a multilevel directory structure for a given file name
    * based on the configured "levels" inside the `opium_path_t` structure.
    *
    * Unlike opium_create_full_path(), which creates directories for an explicit path
    * (e.g. "/tmp/a/b/c"), this function creates subdirectories dynamically based
    * on parts of the *file name*, following Nginx-style "hashed directory" logic.
    *
    * Example:
    *   path.name = "/tmp/yes"
    *   path.level = {2, 1, 0}
    *   file.name  = "/tmp/yes/a1b2c3.tmp"
    *
    * The resulting structure will be:
    *   /tmp/yes/pidr/a1b
    *   /tmp/yes/pidr/a1b/c
    *
    * This helps distribute files evenly across directories (avoiding thousands of files in one folder).
    */
   size_t       pos;
   u_char       buff[OPIUM_FILE_BUFF_SIZE];
   opium_uint_t err;

   opium_memzero(buff, OPIUM_FILE_BUFF_SIZE);
   opium_cpystrn(buff, file->name.data, OPIUM_FILE_BUFF_SIZE - 1);

   /* The base offset is the length of the base directory name. */
   pos = path->name.len;

   /*
    * Iterate through path->level[].
    * Each level defines how many characters from the filename
    * to use for one directory layer.
    */
   for (opium_uint_t index = 0; index < OPIUM_MAX_PATH_LEVEL; index++) {
      if (path->level[index] == 0) {
         break;
      }

      pos = pos + path->level[index] + 1;

      /*
       * Temporarily truncate the path to create the current directory.
       * The '\0' ends the string so mkdir sees a valid directory name.
       */
      buff[pos] = '\0';

      printf("%s\n", buff);

      /*
       * Try to create the directory. If it already exists (EEXIST),
       * just continue. Otherwise, return the error code.
       */
      if (mkdir((const char *)buff, 0700) == -1) {
         err = errno;
         if (err != EEXIST) {
            return err;
         }
      }

      buff[pos] = '/';
   }

   return OPIUM_RET_OK;
}

   opium_uint_t
opium_create_full_path(u_char *dir, opium_uint_t access)
{
   /* 
    * This function incrementally creates all subdirectories in the path if they don't already exist.
    *
    * Example:
    *   /tmp/a/b/c
    *
    *   will result in:
    *     mkdir /tmp/a
    *     mkdir /tmp/a/b
    *     mkdir /tmp/a/b/c
    *
    * Existing directories are skipped.
    */


   u_char       buff[OPIUM_FILE_BUFF_SIZE], *pos;
   opium_uint_t err;

   opium_memzero(buff, OPIUM_FILE_BUFF_SIZE);
   opium_cpystrn(buff, dir, OPIUM_FILE_BUFF_SIZE - 1);

   /*
    * Why not from the very beginning?
    * Because in Unix paths, the first character is the root slash /,
    * and it can't be "cut off" or created. 
    */

   for (pos = buff + 1; *pos; pos++) {
      if (*pos != '/') {
         continue;
      }

      *pos = '\0';

      /*
       * At this point, pos points to the '/' between path components.
       * Example: dir == "/tmp/a/b/c"
       * Contents:
       * [ '/' 't' 'm' 'p' '/' 'a' '/' 'b' '/' 'c' '\0' ]
       *                    ^ pos
       *
       * By setting *pos = '\0', we temporarily "truncate" the string at this point,
       * getting the substring from the beginning to the current position:
       * [ '/' 't' 'm' 'p' '\0' 'a' '/' 'b' '/' 'c' '\0' ]
       *    ^-- is now the string "/tmp" (which is passed to mkdir)
       */
      if (mkdir((const char *)buff, access) == -1) {
         err = errno;

         if (err != EEXIST) {
            return err;
         }

      }

      *pos = '/';

   }

   return OPIUM_RET_OK;
