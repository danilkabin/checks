#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define BIT_S 64
#define min(a,b) ((a) < (b) ? (a) : (b))

void print_mask(uint64_t *mask, size_t size, const char *name) {
   printf("name: %s\n", name);
   for (size_t index = 0; index < size; index++) {
      uint64_t number = mask[index];
      for (int bit = 63; bit >= 0; bit--) {
         printf("%s", (number & (1ULL << bit)) ? "1" : "0");
         if (bit % 8 == 0) printf(" ");
      }
      printf("\n");
   }
   printf("end name: %s\n", name);
}

void result(uint64_t *mask, size_t size, int start_pos, int offset, int next_pos) {
   printf("offset: %d\n", offset);
   printf("start_pos: %d\n", start_pos);
   int offset_count = ((start_pos + offset) / 64) + 1;
   printf("offset_count: %d\n\n", offset_count);

   int end_pos = start_pos + offset;

   int bits_left = offset;
   int current_pos = start_pos;
   int current_n_pos = next_pos;

   uint64_t tmp[size];
   memcpy(tmp, mask, sizeof(uint64_t) * size);

   while (bits_left > 0) {
      int n_index = current_pos / BIT_S;
      int local_pos = current_pos % BIT_S;

      int bits_left_in_word = BIT_S - local_pos;
      int can_clear = min(bits_left, bits_left_in_word);

      uint64_t hermit = (can_clear == 64) ? ~0ULL : ((1ULL << can_clear) - 1);
      tmp[n_index] &= ~(hermit << local_pos);

      bits_left = bits_left - can_clear;
      current_pos = current_pos + can_clear;
   }

   current_pos = start_pos;
   bits_left = offset;

   while (bits_left > 0) {
      int index = current_pos / BIT_S;
      int n_index = current_n_pos / BIT_S;

      int bits_left_in_src_word = 64 - (current_pos % 64);
      int bits_left_in_dst_word = 64 - (current_n_pos % 64);
      int can_write = min(bits_left, min(bits_left_in_src_word, bits_left_in_dst_word));

      printf("index: %d, next_index: %d, can_write: %d\n",
            index, n_index, can_write);

      printf("bits_left: %d, current_pos: %d, current_next_pos:%d\n"
            , bits_left, current_pos, current_n_pos);

      int local_pos = current_pos % BIT_S;
      int local_n_pos = current_n_pos % BIT_S;

      uint64_t current = mask[index];
      uint64_t n_current = mask[n_index]; 

      uint64_t hermit = (can_write == 64) ? ~0ULL : (1ULL << can_write) - 1;

      uint64_t copy_hermit = hermit << local_pos;
      uint64_t extracted = copy_hermit & current;

      uint64_t n_hermit = (can_write == 64) ? ~0ULL : ((1ULL << can_write) - 1) << local_n_pos;

      tmp[n_index] = tmp[n_index] & ~n_hermit;

      uint64_t shifted = (extracted >> local_pos) << local_n_pos;
      tmp[n_index] = tmp[n_index] | shifted;

      bits_left = bits_left - can_write;
      current_pos = current_pos + can_write;
      current_n_pos = current_n_pos + can_write;
   }

   print_mask(tmp, size, "tmp");

   for (size_t index = 0; index < size; index++) {
      mask[index] = tmp[index];
   }
}

int main() {
    uint64_t mask[6];
    size_t mask_size = sizeof(mask) / sizeof(mask[0]);

    int test_num = 5;

    uint64_t test_dfgsd = 0;
    for (int index = 63; index >= 0; index--) {
    test_dfgsd = test_dfgsd | (1ULL << index); 
    }
     
    for (int index = 63; index >= 0; index--) {
       printf("%s", (test_dfgsd & (1ULL << index)) ? "1" : "0");
    }
    printf("number: %ld\n", test_dfgsd);
    printf("\n");
    printf("\n");
    printf("\n");

    switch (test_num) {
        case 1: {
            memset(mask, 0xFF, mask_size * sizeof(uint64_t));             int start_pos = 10;
            int offset = 0;
            int next_pos = 20;

            print_mask(mask, mask_size, "mask before zero offset");
            result(mask, mask_size, start_pos, offset, next_pos);
            print_mask(mask, mask_size, "mask after zero offset");
            break;
        }
        case 2: {
            memset(mask, 0, mask_size * sizeof(uint64_t));
            mask[0] = 1ULL << 5; // бит 5 = 1
            int start_pos = 5;
            int offset = 1;
            int next_pos = 20;

            print_mask(mask, mask_size, "mask before 1 bit copy");
            result(mask, mask_size, start_pos, offset, next_pos);
            print_mask(mask, mask_size, "mask after 1 bit copy");
            break;
        }
        case 3: {
            memset(mask, 0, mask_size * sizeof(uint64_t));
            for (int i = 60; i <= 68; i++) {
                int idx = i / BIT_S;
                int bit = i % BIT_S;
                mask[idx] |= 1ULL << bit;
            }
            int start_pos = 60;
            int offset = 9;
            int next_pos = 10;

            print_mask(mask, mask_size, "mask before cross-boundary copy");
            result(mask, mask_size, start_pos, offset, next_pos);
            print_mask(mask, mask_size, "mask after cross-boundary copy");
            break;
        }
        case 4: {
            memset(mask, 0xFFFFFFF, mask_size * sizeof(uint64_t)); // все 1
            int start_pos = 5;
            int offset = 10;
            int next_pos = 60;

            print_mask(mask, mask_size, "mask before overwrite");
            result(mask, mask_size, start_pos, offset, next_pos);
            print_mask(mask, mask_size, "mask after overwrite");
            break;
        }
        case 5: {
            memset(mask, 0, mask_size * sizeof(uint64_t));
            for (int i = 10; i < 40; i++) {
                int idx = i / BIT_S;
                int bit = i % BIT_S;
                mask[idx] |= 1ULL << bit;
            }
            int start_pos = 10;
            int offset = 20;
            int next_pos = 25;  
            print_mask(mask, mask_size, "mask before overlapping copy");
            result(mask, mask_size, start_pos, offset, next_pos);
            print_mask(mask, mask_size, "mask after overlapping copy");
            break;
        }
        default:
            printf("Unknown test number\n");
    }

    return 0;
}

