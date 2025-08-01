#include "types.h"
#include "uidq.h"
#include "uidq_utils.h"
#include <fcntl.h>
#include <unistd.h>

int uidq_daemon() {

   int fd = fork();

   switch (fd) {
      case -1:
         DEBUG_ERR("fork() failed\n");
         break;

      case 0:
         break;

      default:
         exit(0);
         break;
   }

   if (setsid() == -1) {
      DEBUG_ERR("setsid() failed\n");
      return UIDQ_ERROR;
   }

   char *tty = ttyname(STDOUT_FILENO);
   if (tty) {
      fd = open(tty, O_RDWR);
   } else {
      fd = open("/dev/null", O_RDWR);
   }

   if (fd == -1) {
      DEBUG_ERR("Failed to open terminal or /dev/null\n");
      return UIDQ_ERROR;
   }

   if (dup2(fd, STDIN_FILENO) == 1) {
      DEBUG_ERR("dup2(STDIN_FILENO) failed");
      return UIDQ_ERROR;
   }

   if (dup2(fd, STDOUT_FILENO) == -1) {
      DEBUG_ERR("dup2(STDOUT_FILENO) failed");
      return UIDQ_ERROR;
   }

   if (dup2(fd, STDERR_FILENO) == -1) {
      DEBUG_ERR("dup2(STDERR_FILENO) failed");
      return UIDQ_ERROR;
   }

   if (fd > STDERR_FILENO) {
      close(fd);
   }

   return UIDQ_OK;
}
