#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "uidq_utils.h"

int uidq_round_pow(int number, int exp) {
   if (number < 1 || exp < 2) {
      DEBUG_ERR("Invalid input: number (%d) must be >= 1 and exp (%d) must be >= 2.\n", number, exp);
      return -1;
   }

   int real = 1;
   while (real < number) {
      if (real > INT_MAX / exp) {
         DEBUG_ERR("Overflow detected: real (%d) * exp (%d) would exceed INT_MAX.\n", real, exp);
         return -1;
      }
      real = real * exp;
   }
   return real;
}
