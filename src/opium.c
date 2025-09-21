#include "core/opium_alloc.h"
#include "core/opium_bitmask.h"
#include "core/opium_log.h"
#include "core/opium_pool.h"
#include "core/opium_slab.h"
#include "core/opium_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITEM_SIZE 2048
#define MAX_PTRS 1000000  // огромная куча

void print_memory_usage() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0) {
            printf("%s", line);
        }
    }
    fclose(f);
}

int main() {
    srand((unsigned int)time(NULL));

    opium_log_t *log = opium_log_init(NULL, NULL, NULL);
    opium_slab_t slab;
    opium_slab_init(&slab, ITEM_SIZE, log);

    void **ptrs = malloc(sizeof(void*) * MAX_PTRS);
    if (!ptrs) {
        perror("malloc ptrs");
        return 1;
    }

    printf("Memory before allocation:\n");
    print_memory_usage();

    // Аллокация всей кучи без free
    for (size_t i = 0; i < MAX_PTRS; i++) {
        void *p = opium_slab_alloc(&slab);
        if (!p) {
         //   printf("Allocation failed at index %zu\n", i);
            break;
        }
        memset(p, 0xAA, ITEM_SIZE);
        ptrs[i] = p;
        if (i % 100000 == 0) {
       //     printf("Allocated %zu objects\n", i);
        }
    }

    printf("Memory after full allocation:\n");
    print_memory_usage();

    // Статистика slab
  //  opium_slab_stats(&slab);

    // Удаляем все объекты
    for (size_t i = 0; i < MAX_PTRS; i++) {
        if (ptrs[i]) {
            opium_slab_free(&slab, ptrs[i]);
        }
    }

    free(ptrs);

    printf("Memory after freeing all objects:\n");
    print_memory_usage();

    opium_slab_exit(&slab);

    printf("Memory after slab exit:\n");
    print_memory_usage();

    return 0;
}
