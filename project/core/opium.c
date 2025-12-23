#include "core/opium.h"

   int 
opium_is_little_endian(void)
{

   /* We want to determine the order of bytes in memory - endianness. 
    * Variable: uint16_t takes up 2 bytes.
    * The value 1 can be stored in memory in different ways:
    * Little-endian   low byte    01 00
    * Big-endian      high byte   00 01
    */

   uint16_t x = 1;

   /* &x - we take the adress of the variable x 
    * (a pointer to a 16-bit number)
    *
    * The value is the contents of that byte in memory:
    * In little-endian, the least significant byte is stored first:
    * Address :   0x00     0x01  
    * Value   :   0x01     0x00
    * In big-endian, the least significant byte is stored second:
    * Address :   0x00     0x01
    * Value   :   0x00     0x01
    *
    * Value and Addres difference:
    * Address: the location of a byte in memory, 0x00, 0x01, 0x02 ...
    * Value: the content stored at that memory location,
    * for example, the value 0x01 is stored at address 0x02.
    */

   uint8_t *byte = (uint8_t*)&x;

   /* &x is a pointer to uint16_t, which is two-byte variable
    * If we want to look byte to byte, we must use a pointer to one byte (uint8_t)
    * (uint8_t*)&x;
    * Now we can dereference this pointer to get the first byte:
    */

   return *byte == 1;
}

   size_t 
opium_round_of_two(size_t x)
{
   /*
    * Example: rounding up to the nearest power of two
    * Let's use an 8-bit register for example.
    * Let size = 32
    *
    * Step 1: convert to binary
    *  - 32 = 0b00100000
    *
    * Step 2: subtract 1
    *  - 32 - 1 = 0b00011111
    *
    * Step 3: Count leading zeroes (clz):
    *   0b00011111
    *     ^^^
    *   lz = 3
    *
    * Step 4: Compute the next power of two:
    * 1 << (8 - lz)
    * 1 << (8 - 3)
    * 1 << 5 = 32
    *
    * Note:
    *   For 32-bit numbers, replace 8 with 32;
    *   For 64-bit numbers, replace 8 with 64.
    */
#if (OPIUM_PTR_SIZE == 4)
   size_t lz = __builtin_clzl(x - 1);
   return 1UL << (32 - lz);
#else
   size_t lz = __builtin_clzll(x - 1);
   return 1ULL << (64 - lz);
#endif

}

   size_t 
opium_log2(size_t x)
{
   /*
    * Compute the integer log2 of x
    * 
    * Example (8-bit for simplicity):
    *   x = 20
    *   binary: 00010100   
    * 
    * Step 1: Count leading zeroes
    *   clz(20) = 3 00010100
    *               ^^^ (zeroes 3 before first 1) 
    *
    * Step 2: Subtract from total bits - 1
    *   8 Bit: 7 - 3 = 4
    *
    *  Result:
    *  log(20) = 4
    *  2^4 = 16 <= 20 < 2^5 = 32
    *
    */
#if (OPIUM_PTR_SIZE == 4)
   return 31 - __builtin_clz(x);
#else
   return 63 - __builtin_clzll(x);
#endif
}

   void
opium_debug_point(void)
{
   raise(SIGSTOP);
}
