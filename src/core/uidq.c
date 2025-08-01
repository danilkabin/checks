#include "uidq.h"
#include "../master/uidq_master.h"
#include "misc/uidq_utils.h"
#include "types.h"
#include "uidq_config.h"
#include <stdint.h>
#include <time.h>
#include <unistd.h>

static int uidq_show_help = 0;
static int uidq_show_version = 0;
static int uidq_show_configure = 0;
static int uidq_merci_mode = 0;

static int uidq_daemon_enable = 1;

static int uidq_get_argv(int argc, char *argv[]);
static void uidq_show_info(void); 

static void uidq_show_environment();
static int uidq_set_environment();

static int quiiit = 0;

// EPOLL + SOCKET!! and listening commands
// EPOLL + SOCKET!! and listening commands
// EPOLL + SOCKET!! and listening commands
// EPOLL + SOCKET!! and listening commands

static uidq_server_config_t server_conf = {
   .addr = UIDQ_DEFAULT_ADDR,
   .port = UIDQ_DEFAULT_PORT,
   .max_peers = UIDQ_DEFAULT_MAX_PEERS,
   .max_queue = UIDQ_DEFAULT_MAX_QUEUE,
   .log_file = UIDQ_DEFAULT_LOG_FILE  
};

int main(int argc, char *argv[]) {
   DEBUG_FUNC("sfdsdfs\n");

   if (uidq_get_argv(argc, argv) != UIDQ_OK) {
      return 1;
   }

   if (uidq_daemon_enable) {
      if (uidq_daemon() != UIDQ_OK) {
         exit(0);
      }
   } 

   if (uidq_show_version) {
      uidq_show_info();
   }

   if (uidq_show_configure) {
      uidq_show_environment();
   }

   DEBUG_FUNC("pid_t: %ld\n", (long)getpid());

   time_t timer = time(NULL);
   if (timer == (time_t)(-1)) {
      DEBUG_ERR("Failed timer\n");
      return 1;
   }

   while (1) {
      time_t current_time = time(NULL);
      if (current_time == (time_t)(-1)) {
         DEBUG_ERR("Failed timer\n");
         return 1;
      }

      int elapsed = difftime(current_time, timer);
      
      if (elapsed > 3) {
         quiiit = true;
      }

      if (quiiit) {
         exit(0);
      }
      sleep(1);
   }

   return 1;
}

int uidq_get_argv(int argc, char *argv[]) {
   char *pos;
   for (int index = 1; index < argc; index++) {
      pos = (char*)argv[index];

      if (*pos++ != '-') {
         DEBUG_ERR("Invalid option: (%s)\n", argv[index]);
         return UIDQ_ERROR;
      }

      while (*pos) {
         switch(*pos) {
            case '?':
            case 'h':
               uidq_show_help = 1;
               uidq_show_version = 1;
               break;

            case 'v':
               uidq_show_version = 1;
               break;

            case 'q':
               uidq_merci_mode = 1;
               break;

            case 's':
               uidq_show_configure = 1;
               break;

            case 'a':

               break;

            case 'f':
               uidq_daemon_enable = 0; 
               break;

            default:
               DEBUG_ERR("Invalid option: (%s)\n", argv[index]);
               return UIDQ_ERROR;
         }
         pos++;
      }

      DEBUG_FUNC("argv:%s\n", argv[index]);
   }

   return UIDQ_OK; 
}

static void uidq_show_info(void) {
   DEBUG_INFO("uidq version: " UIDQ_VER "\n");

   if (uidq_show_help) {
      DEBUG_INFO(
            "Usage: uidq [options]\n"
            "Options:\n"
            "  -h, -?       : show this help and exit\n"
            "  -v           : show version and exit\n"
            "  -s           : configure the server\n"
            "  -q           : quiet mode\n"
            );
   }
}

static void uidq_show_environment() {
   DEBUG_INFO(
         "Usage: uidq [options]\n"
         "Options:\n"
         "  -m           : Configure master worker process (main controller)\n"
         "  -p           : Start process worker (handles specific tasks)\n"
         "  -l           : Save current configuration to file\n"
         "  -q           : Quiet mode, suppress non-error output\n"
         "\n"
         );
}

static int uidq_set_environment() {

   return UIDQ_OK;
}
