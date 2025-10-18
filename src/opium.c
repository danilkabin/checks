/*
 * Test program for the opium_hash module.
 *
 * Modifications:
 * - Added detailed logging for each test case.
 * - Added assertions for list integrity after deletions.
 * - Ensured proper cleanup with opium_log_exit.
 * - Kept original test structure, focusing on robustness and crash prevention.
 */

#include "core/opium_alloc.h"
#include "core/opium_arena.h"
#include "core/opium_conf_file.h"
#include "core/opium_core.h"
#include "core/opium_file.h"
#include "core/opium_hash.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include "core/opium_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Print memory usage from /proc/self/status */
   static void
print_memory_usage(opium_log_t *log)
{
   FILE *f = fopen("/proc/self/status", "r");
   if (!f) {
      opium_log_err(log, "Failed to open /proc/self/status for memory usage.\n");
      return;
   }

   char line[256];
   while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0) {
         printf("%s", line);
      }
   }
   fclose(f);
}

   int
main(void)
{
   opium_log_t *log = opium_log_init(NULL, NULL, NULL);
   if (!log) {
      fprintf(stderr, "Failed to initialize logging.\n");
      return 1;
   }

   /*   opium_path_t path;
        opium_file_t file;

        path.name.data = (u_char*)"/home/dima/Projects/yes/pidr/";
        path.name.len = opium_strlen(path.name.data);
        path.access = 0777;

        if (opium_create_full_path(path.name.data, path.access) != OPIUM_RET_OK) {
        fprintf(stderr, "[-] Failed to create base path\n");
        return 1;
        }

        path.level[0] = 2;
        path.level[1] = 1;
        path.level[2] = 0;

        file.fd        = -1;
        file.name.data = (u_char*)"/home/dima/Projects/yes/pidr/a1b2c3.tmp";
        file.name.len  = opium_strlen(file.name.data);
        file.log       = log;

        printf("\n[CREATE_PATH] File: %s\n", file.name.data);
        opium_create_optimized_path(&path, &file);

        file.name.data = (u_char*)"/home/dima/Projects/yes/pidr/f7e9aa.tmp";
        file.name.len  = opium_strlen(file.name.data);

        printf("\n[CREATE_PATH] File: %s\n", file.name.data);
        opium_create_optimized_path(&path, &file);
        */   
   opium_pool_t pool;
   opium_pool_init(&pool, 100, log);

   const char *path_str = "/home/dima/Projects/norm3/etc/opium.conf";
   size_t siize = strlen(path_str) + 1;

   u_char yes[siize];
   opium_cpystrn(yes, (u_char*)path_str, siize);

   opium_open_file_t open_file;
   /*
      opium_conf_t *conf = &open_file.conf;

      opium_command_t commands[] = {
      { .name = opium_string("server"),
      .type = OPIUM_COMMAND_SERVER_TYPE,
      .set = opium_conf_set_str,
      .conf = &conf->server_conf,
      .offset = offsetof(opium_conf_server_t, server_name) },

      { .name = opium_string("ip"),
      .type = OPIUM_COMMAND_SERVER_TYPE,
      .set = opium_conf_set_str,
      .conf = &conf->server_conf,
      .offset = offsetof(opium_conf_server_t, ip) },

      { .name = opium_string("port"),
      .type = OPIUM_COMMAND_SERVER_TYPE,
      .set = opium_conf_set_num,
      .conf = &conf->server_conf,
      .offset = offsetof(opium_conf_server_t, port) },

      { .name.len = 0 } 
      };
      */
   //opium_log_debug(log, "server: %s\n", conf->server_conf.server_name.data); 
  // opium_log_debug(log, "ip: %d\n", conf->server_conf.ip.data); 
  // opium_log_debug(log, "port: %d\n", conf->server_conf.port); 


   return 0;
}
