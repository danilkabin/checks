#include "opium.h"
#include <signal.h>
#include <stdint.h>

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

   void
opium_debug_point(void)
{
   raise(SIGSTOP);
}
