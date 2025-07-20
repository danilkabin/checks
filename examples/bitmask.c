#include "../src/core/uidq.h"

#include "misc/uidq_bitmask.h"
#include "misc/uidq_utils.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define ASSERT(cond, msg) \
   do { \
      if (!(cond)) { \
         fprintf(stderr, "ASSERT FAILED: %s\n", msg); \
         exit(EXIT_FAILURE); \
      } \
   } while (0)

/*static void scenario_basic_set_clear(uidq_bitmask_t *bm) {
  printf("[+] Scenario: Basic set and clear single bit\n");
  int pos = uidq_find_first_bit(bm, 0); // find first free bit
  ASSERT(pos >= 0, "No free bit found to set");

  uidq_bit_set(bm, pos);
  ASSERT(uidq_bitmask_bit_test(bm, 1, pos, 1) >= 0, "Bit should be set after uidq_bit_set");
  ASSERT(bm->bit_count == 1, "bit_count should be 1 after setting");

  uidq_bit_free(bm, pos);
  ASSERT(uidq_bitmask_bit_test(bm, 0, pos, 1) >= 0, "Bit should be cleared after uidq_bit_free");
  ASSERT(bm->bit_count == 0, "bit_count should be 0 after clearing");
  }

  static void scenario_bulk_set_clear(uidq_bitmask_t *bm) {
  printf("[+] Scenario: Bulk set and clear multiple bits\n");
  int pos = uidq_find_grab_bit(bm, 10, 0); // find 10 consecutive free bits
  ASSERT(pos >= 0, "Failed to find 10 consecutive free bits");

  for (int i = pos; i < pos + 10; ++i)
  uidq_bit_set(bm, i);
  ASSERT(bm->bit_count == 10, "bit_count should be 10 after bulk set");

  for (int i = pos; i < pos + 10; ++i)
  uidq_bit_free(bm, i);
  ASSERT(bm->bit_count == 0, "bit_count should be 0 after bulk clear");
  }

  static void scenario_find_and_grab(uidq_bitmask_t *bm) {
  printf("[+] Scenario: Find grab of free bits after setting some bits\n");
  uidq_bit_set(bm, 0);
  uidq_bit_set(bm, 1);
  uidq_bit_set(bm, 5);

  int pos = uidq_find_grab_bit(bm, 3, 0);
  ASSERT(pos >= 0, "Failed to find 3 consecutive free bits");
  printf("Found free sequence at position %d\n", pos);

// cleanup
uidq_bit_free(bm, 0);
uidq_bit_free(bm, 1);
uidq_bit_free(bm, 5);
}

static void scenario_bitmask_invert(uidq_bitmask_t *bm) {
printf("[+] Scenario: Invert bitmask\n");
ASSERT(bm->bit_count == 0, "Bitmask should start empty");

uidq_bitmask_invert(bm);
ASSERT(bm->bit_count == bm->bit_capacity, "After invert all bits should be set");

uidq_bitmask_invert(bm);
ASSERT(bm->bit_count == 0, "Invert back: bit_count should be 0 again");
}

static void scenario_bitmask_op(uidq_bitmask_t *bm1, uidq_bitmask_t *bm2, uidq_bitmask_t *result) {
printf("[+] Scenario: Bitwise operations (AND, OR)\n");
uidq_bit_set(bm1, 2);
uidq_bit_set(bm2, 2);
uidq_bit_set(bm2, 3);

int ret = uidq_bitmask_op(result, bm1, bm2, UIDQ_BITMASK_OP_AND);
ASSERT(ret == 0, "Bitmask AND failed");
ASSERT(result->bit_count == 1, "AND should have 1 bit set");

ret = uidq_bitmask_op(result, bm1, bm2, UIDQ_BITMASK_OP_OR);
ASSERT(ret == 0, "Bitmask OR failed");
ASSERT(result->bit_count == 2, "OR should have 2 bits set");

// cleanup
uidq_bit_free(bm1, 2);
uidq_bit_free(bm2, 2);
uidq_bit_free(bm2, 3);
}
*/
int main(void) {
   printf("== Running uidq_bitmask test ==\n");

   uidq_bitmask_t *bm = uidq_bitmask_create_and_init(128, sizeof(uint64_t));
   ASSERT(bm != NULL, "Failed to create bitmask");

   /*  scenario_basic_set_clear(bm);
       scenario_bulk_set_clear(bm);
       scenario_find_and_grab(bm);
       scenario_bitmask_invert(bm);
       */
   uidq_bitmask_t *bm2 = uidq_bitmask_create_and_init(128, sizeof(uint64_t));
   uidq_bitmask_t *result = uidq_bitmask_create_and_init(128, sizeof(uint64_t));
   ASSERT(bm2 != NULL && result != NULL, "Failed to create bitmasks for bitmask_op");

   //   scenario_bitmask_op(bm, bm2, result);

   mode_t flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IRGRP | S_IROTH | S_IWOTH;
   int file = open("text.txt", O_CREAT | O_TRUNC | O_EXCL, flags);
   if (file < 0) {
      DEBUG_ERR("Fail!\n"); 
      return -1;
   } 
   DEBUG_FUNC("Hello!\n");
   uidq_bitmask_save(bm, file);

   uidq_bitmask_free(bm);
   uidq_bitmask_free(bm2);
   uidq_bitmask_free(result);

   printf("== All tests passed successfully! ==\n");
   return 0;
}
