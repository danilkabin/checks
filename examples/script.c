#include "../src/core/uidq.h"

#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

int main() {
   uidq_module_init(); 
   sleep(100);
   return 0;
}
