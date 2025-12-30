#include "app/opium_main.h"
#include "core/opium_arena.h"
#include "core/opium_core.h"
#include "core/opium_event.h"
#include <bits/pthreadtypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

void onn(opium_fd_t fd, opium_u32_t mask, void *ctx) {
   printf("changed!\n");
}

int main() {
   opium_log = opium_log_init(NULL, NULL, NULL);
   if (!opium_log) {
      fprintf(stderr, "Failed to initialize logging.\n");
      return 1;
   }

   opium_log_debug(opium_log, "hello niggers!\n");

   char *misha = "misha pidoras";
   printf("misha: %s\n", misha);

   for (size_t byte = 0; byte < strlen(misha); byte++) {
      u_char ch = (u_char)misha[byte];
      for (int bit = 7; bit >= 0; bit--) {
         printf("%d", (ch >> bit) & 1);
      }
      printf(" ");
   }

   opium_arena_t arena;
   int ret = opium_arena_init(&arena, opium_log);

   opium_event_t event;

   return -1;
}
