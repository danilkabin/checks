#include "core/opium_core.h"
#include <stdint.h>
#include <stdio.h>

void opium_cpuinfo() {
   uint32_t eax, ebx, ecx, edx;

   eax = 1;
   __asm__(
         "cpuid"
         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
         : "a"(eax)
         );

   uint32_t family = ((eax >> 8) & 0xf) + ((eax >> 20) & 0xff);
   uint32_t model = ((eax >> 4) & 0xf) + ((eax >> 16) & 0xf0);

       if (edx & (1 << 25)) printf("Supports SSE\n");
    if (edx & (1 << 26)) printf("Supports SSE2\n");
    if (ecx & (1 << 0))  printf("Supports SSE3\n");
    if (ecx & (1 << 9))  printf("Supports SSSE3\n");
    if (ecx & (1 << 19)) printf("Supports SSE4.1\n");
    if (ecx & (1 << 20)) printf("Supports SSE4.2\n");
    if (ecx & (1 << 28)) printf("Supports AVX\n");

   printf("Cpu family: %u, model: %u\n", family, model);

   char vendor[13];

   eax = 0;
   __asm__ (
         "cpuid"
         : "=b"(ebx), "=d"(edx), "=c"(ecx)
         : "a"(eax)
         );

   *(uint32_t*)&vendor[0] = ebx;
   *(uint32_t*)&vendor[4] = edx;
   *(uint32_t*)&vendor[8] = ecx;
   vendor[12] = '\0';

   printf("Model vendor: %s\n", vendor);

}
