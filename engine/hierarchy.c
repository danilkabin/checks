#include "engine/hierarchy.h"
#include "raylib.h"
#include "typedef.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"

// === Global Variables ===

objectSpace *workspace = NULL;
size_t memory_busy = 0;
size_t memory_busyTimes = 0;
size_t memory_clearTimes = 0;

struct hierarchyBitPools_struct {
   bitmaskPool *pool_partCube;
   bitmaskPool *pool_partSphere;
   bitmaskPool *pool_partCylinder;
   bitmaskPool *pool_partMesh;
} hierarchyBitPools;

struct hierarchyUniquePools_struct {
   uniquePool *pool_partStructPool;
} hierarchyUniquePools;

// === Type Definitions ===

typedef struct {
   s_int_32 targetIndex;
   void *part;
} partPoolInit_struct;

typedef void (*RemovePartFunc)(void **part, bitmaskPool *pool, s_int_32 targetIndex);
typedef u_int_32(*bitmaskOperation)(void *data, u_int_32 index, u_int_8 side, u_int_8 variable) ;

static RemovePartFunc removePartFuncs[OBJECT_TYPE_COUNT] = {
   [OBJECT_TYPE_CUBE]     = (RemovePartFunc)remove_cube,
   [OBJECT_TYPE_SPHERE]   = (RemovePartFunc)remove_sphere,
   [OBJECT_TYPE_CYLINDER] = (RemovePartFunc)remove_cylinder,
   [OBJECT_TYPE_MESHPART] = (RemovePartFunc)remove_meshPart
};

static bitmaskOperation bitOperations[BITMASK_COUNT] = {
   [BITMASK_FIRST_BIT]     = (bitmaskOperation)changeFirstBit,
   [BITMASK_CURRENT_BIT]   = (bitmaskOperation)changeCurrentBit,
   [BITMASK_MULTIPLY_BIT]  = (bitmaskOperation)changeMultiplyBit,
};

// === Memory Management ===

#pragma region MEMORY_MANAGMENT

void *readable_malloc(size_t len) {
   if (len == 0) return NULL;
   void *ptr = malloc(len);
   if (ptr) {
      memory_busy += len;
      memory_busyTimes += 1;
   }
   printf("Memory allocated at: %p, Size: %zu, Total memory usage: %zu\n", ptr, len, memory_busy);
   return ptr;
}

void *readable_realloc(void *ptr, size_t len, size_t oldlen) {
   if (!ptr || len == 0) return NULL;
   void *newptr = realloc(ptr, len);
   if (newptr) {
      memory_busy = memory_busy - oldlen + len;
      memory_busyTimes += 1;
   }
   return newptr;
}

void free_readable_malloc(void *ptr, size_t len) {
   if (!ptr) return;
   free(ptr);
   memory_busy = (memory_busy < len) ? 0 : memory_busy - len;
   memory_clearTimes += 1;
   printf("Memory freed: %zu bytes, Total memory usage: %zu\n", len, memory_busy);
}

char *str_dup(const char *str) {
   size_t len = strlen(str) + 1;
   char *newStr = readable_malloc(len);
   if (newStr) {
      strcpy(newStr, str);
   }
   return newStr;
}

void changePathVar(char **ptr, const char *str) {
   if (!ptr || !str) return;
   if (*ptr) {
      free_readable_malloc(*ptr, strlen(*ptr));
   }
   *ptr = str_dup(str);
}

#pragma endregion

// === Bitmask Pool Utilities ===

#pragma region UTILS_MANAGMENT

s_int_32 check_on_2x(u_int_32 number, u_int_16 multiply) {
#ifdef __GNUC__
   return (1U << (31 - __builtin_clz(number * multiply)));
#else
   u_int_32 defaultNumber = multiply;
   while (defaultNumber < number) {
      defaultNumber <<= 1;
   }
   return defaultNumber;
#endif
}

s_int_32 checkAllBitsOnSet(u_int_64 number, size_t size) {
#ifdef __GNUC__
   if (number == ~0ULL) return RESULT_TRASH;
   return __builtin_ctzll(~number);
#else
   for (s_int_32 indexInt = 0; indexInt < size; indexInt++) {
      if (IS_BIT_SET(number, indexInt) == 0) {
         return (int)indexInt;
      }
   }
   return RESULT_TRASH;
#endif
}

void printBits(u_int_64 val, s_int_32 bits) {
   for (s_int_32 i = bits - 1; i >= 0; i--) {
      printf("%d", (val >> i) & 1);
   }
   printf("\n");
}

s_int_32 changeFirstBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable) {
   u_int_64 *bitArray = (u_int_64 *)data;
   s_int_32 targetIndex = RESULT_TRASH;
   for (u_int_32 inx = 0; inx < index; inx++) {
      u_int_32 bit = checkAllBitsOnSet(bitArray[inx], POOL_BITMASK_SIZE);
      if (bit == RESULT_TRASH) continue;
      bitArray[inx] = (side == 1) ? (bitArray[inx] | (1UL << bit)) :
         (bitArray[inx] & ~(1UL << bit));
      //  printBits(bitArray[inx], POOL_BITMASK_SIZE);
      targetIndex = inx * POOL_BITMASK_SIZE + bit;
      break;
   }
   return targetIndex;
}

s_int_32 changeCurrentBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable) {
   u_int_64 *bitArray = (u_int_64 *)data;
   s_int_32 targetIndex = RESULT_TRASH;
   u_int_16 localIndex = index / POOL_BITMASK_SIZE;
   u_int_16 localCurrent = index - POOL_BITMASK_SIZE * localIndex;

   bitArray[localIndex] = (side == 1) ? (bitArray[localIndex] | (1UL << localCurrent)) :
      (bitArray[localIndex] & ~(1UL << localCurrent));
   //printBits(bitArray[localIndex], POOL_BITMASK_SIZE);
   return targetIndex;
}

s_int_32 changeMultiplyBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable) {
   u_int_64 *bitArray = (u_int_64 *)data;
   s_int_32 targetIndex = RESULT_TRASH;
   u_int_32 counter = 0;

   for (s_int_32 inx = 0; inx < index; inx++) {
      for (s_int_32 inx2 = 0; inx2 < POOL_BITMASK_SIZE; inx2++) {
         if (((bitArray[inx] >> inx2) & 1) != side) {
            counter = counter + 1;
            if (counter == variable) {
               targetIndex = (inx * POOL_BITMASK_SIZE + inx2) - (variable - 1);

               for (s_int_32 bit = 0; bit < variable; bit++) {
                  s_int_32 realBitPos = targetIndex + bit;
                  u_int_32 arrayIndex = realBitPos / POOL_BITMASK_SIZE;
                  u_int_32 bitPos = realBitPos % POOL_BITMASK_SIZE;

                  bitArray[arrayIndex] = (side == 1) ? (bitArray[arrayIndex] |= (1UL << bitPos)) : (bitArray[arrayIndex] &= ~(1UL << bitPos));
               }

               return targetIndex; 
            }
         } else {
            counter = 0; 
         }
      }
   } 
   return targetIndex;
}


s_int_32 setPartPoolIndex(bitmaskPool *pool, void *part, size_t size) {
   if (!pool || !part || size < 0) {
      LOG_ERR("setPartPoolIndex: Invalid pool data");
      return RESULT_TRASH;
   }

   s_int_32 targetIndex = bitmaskPool_add(pool, part, size);
   return targetIndex;
}


void *get_mem_place(void *pool, s_int_32 targetIndex, size_t size, bool isUniquePool) {
   if (!pool || targetIndex < 0) {
      LOG_ERR("get_mem_place: Invalid pool data");
      return NULL;
   }
   if (isUniquePool) {
      bitmaskPool *bPool = (bitmaskPool *)pool;
      return (void *)((u_int_8 *)bPool->memory + targetIndex * size);
   } else {
      uniquePool *uPool = (uniquePool *)pool;
      return (void *)((u_int_8 *)uPool->memory + targetIndex * size);
   }
}

partPoolInit_struct initPartPool(bitmaskPool *pool, void *part, size_t size) {
   partPoolInit_struct res = { .targetIndex = RESULT_TRASH, .part = NULL };
   u_int_32 targetIndex = setPartPoolIndex(pool, part, size);
   void *gottenPart = (targetIndex >= 0) ? get_mem_place(pool, targetIndex, pool->sizePerFrame, false) : NULL;
   if (gottenPart != NULL) {
      res.targetIndex = targetIndex;
      res.part = gottenPart;
   }
   return res;
}

void *cpyByTargetIndex(void *data, void *mem, u_int_32 period, s_int_32 targetIndex, u_int_32 offset) {
   /*if (!data || !mem || targetIndex <= 0) {
     LOG_ERR("Invalid parameters in cpyByTargetIndex");
     return NULL;
     }*/
   void *mem_place = (void *)((u_int_8 *)mem + targetIndex * period);
   memcpy(mem_place, data, period * offset);
   printf("convert: %p", mem_place);
   return mem_place;
}

#pragma endregion

// === Bitmask Pool Management ===

#pragma region BITMASK_POOL_MANAGMENT

bitmaskPool *bitmaskPool_init(size_t size, size_t sizePerFrame) {
   if (size <= 0 || sizePerFrame <= 0 || size < sizePerFrame) {
      LOG_ERR("Invalid parameters in bitmaskPool_init()");
      return NULL;
   }

   bitmaskPool *pool = readable_malloc(sizeof(bitmaskPool));
   if (!pool) {
      LOG_ERR("Failed to allocate memory for bitmaskPool in bitmaskPool_init()");
      goto unsuccessful;
   }

   if (size < (sizePerFrame * POOL_BITMASK_SIZE)) {
      size = sizePerFrame * POOL_BITMASK_SIZE;
   }

   pool->memory = NULL;
   pool->cellVar = NULL;
   pool->size = check_on_2x(size - 1, 2);
   pool->sizePerFrame = sizePerFrame;
   pool->capacity = ROUND_UP(pool->size / sizePerFrame, POOL_BITMASK_SIZE);
   pool->cellCount = 0;
   pool->isActive = false;

   pool->memory = readable_malloc(pool->size);
   if (!pool->memory) {
      LOG_ERR("Failed to allocate memory for pool->memory in bitmaskPool_init()");
      goto unsuccessful;
   }
   memset(pool->memory, 0, pool->size);

   pool->cellVar = readable_malloc((POOL_BITMASK_SIZE / 8) * pool->capacity);
   if (!pool->cellVar) {
      LOG_ERR("Failed to allocate memory for pool->cellVar in bitmaskPool_init()");
      goto unsuccessful;
   }
   memset(pool->cellVar, 0, (POOL_BITMASK_SIZE / 8) * pool->capacity);

   pool->isActive = true;
   LOG_INFO("bitmaskPool initialized successfully");
   return pool;

unsuccessful:
   bitmaskPool_clear(pool);
   pool = NULL;
   return NULL;
}

void bitmaskPool_clear(bitmaskPool *pool) {
   if (!pool) {
      LOG_ERR("Invalid data in bitmaskPool_clear");
      return;
   }
   if (pool->memory) {
      free(pool->memory);
      pool->memory = NULL;
      LOG_WARN("Cleared pool memory");
   }
   if (pool->cellVar) {
      free(pool->cellVar);
      pool->cellVar = NULL;
      LOG_WARN("Cleared pool cell variables");
   }
   free(pool);
}

bitmaskPool *bitmaskPool_rewrite(bitmaskPool *pool, u_int_8 side) {
   if (!pool || !pool->isActive) {
      LOG_ERR("Invalid pool data in bitmaskPool_rewrite");
      return NULL;
   }

   size_t oldsize = pool->size;
   u_int_16 oldCellCount = pool->cellCount;
   u_int_32 oldcapacity = pool->capacity;

   pool->size = (side == 1) ? pool->size * 2 : pool->size / 2;
   pool->capacity = ROUND_UP(pool->size / pool->sizePerFrame, POOL_BITMASK_SIZE);
   printf("bitmaskPool_rewrite: oldsize=%zu, newsize=%zu, oldcapacity=%u, newcapacity=%u\n",
         oldsize, pool->size, oldcapacity, pool->capacity);
   void *newMem = readable_realloc(pool->memory, pool->size, oldsize);
   if (!newMem) {
      LOG_ERR("Failed to reallocate memory in bitmaskPool_rewrite");
      goto unsuccessful;
   }

   pool->cellVar = readable_realloc(pool->cellVar, (POOL_BITMASK_SIZE / 8) * pool->capacity, POOL_BITMASK_SIZE * oldcapacity);
   if (!pool->cellVar) {
      LOG_ERR("Failed to allocate memory for pool->cellVar in bitmaskPool_init()");
      free(newMem);
      newMem = NULL;
      goto unsuccessful;
      return NULL;
   }

   pool->memory = newMem;
   LOG_INFO("bitmaskPool_rewrite successfully!");
   return pool;
unsuccessful:
   pool->size = oldsize;
   pool->capacity = oldcapacity;
   pool->cellCount = oldCellCount;
   return NULL;
}

s_int_32 bitmaskPool_add(bitmaskPool *pool, void *data, size_t size) {
   if (!pool || !pool->isActive || !data || size <= 0) {
      LOG_ERR("bitmaskPool_add: Invalid data or size");
      return 0;
   }
   if ((pool->cellCount * pool->sizePerFrame) > (pool->size * 0.75)) {
      LOG_ERR("hlell");
      bitmaskPool_rewrite(pool, 1);
   }   
   s_int_32 targetIndex = bitOperations[BITMASK_FIRST_BIT](pool->cellVar, pool->capacity, 1, 0);
   //printf("cell: %d, sizeper: %d, size: %zu\n", pool->cellCount, pool->sizePerFrame, pool->size);

   if (targetIndex != RESULT_TRASH) {
      cpyByTargetIndex(data, pool->memory, pool->sizePerFrame, targetIndex, 1);
      pool->cellCount = pool->cellCount + 1;
   }

   return targetIndex;
}

void bitmaskPool_remove(bitmaskPool *pool, u_int_32 index) {
   if (!pool || !pool->isActive) {
      LOG_ERR("bitmaskPool_remove: Invalid pool data");
      return;
   }
   printf("remove yes yes: %d\n", index);

   bitOperations[BITMASK_CURRENT_BIT](pool->cellVar, index, 0, 0);

   pool->cellCount = pool->cellCount - 1;
   if (pool->cellCount <= 0) {pool->cellCount = 0;}
   s_int_8 empty = 0;
   cpyByTargetIndex(&empty, pool->memory, pool->sizePerFrame, index, 1);
}

#pragma endregion

// === Unique Pool Management ===

#pragma region UNIQUE_POOL_MANAGMENT

uniquePool *uniquePool_init(size_t size, size_t sizePerFreeFrame) {
   if (size <= 0 || sizePerFreeFrame <= 0 || size < sizePerFreeFrame) {
      LOG_ERR("Invalid parameters in uniquePool_init()");
      return NULL;
   }

   uniquePool *pool = readable_malloc(sizeof(uniquePool));
   if (!pool) {
      LOG_ERR("Failed to allocate memory for bitmaskPool in uniquePool_init()");
      goto unsuccessful;
   }

   if (size < (sizePerFreeFrame * POOL_BITMASK_SIZE)) {
      size = sizePerFreeFrame * POOL_BITMASK_SIZE;
   }

   pool->memory = NULL;
   pool->cellVar = NULL;
   pool->size = check_on_2x(size - 1, 2);
   pool->sizePerFreeFrame = sizePerFreeFrame;
   pool->capacity = ROUND_UP(pool->size / sizePerFreeFrame, POOL_BITMASK_SIZE);
   pool->cellCount = 0;

   pool->memory = readable_malloc(pool->size);
   if (!pool->memory) {
      LOG_ERR("Failed to allocate memory for pool->memory in uniquePool_init()");
      goto unsuccessful;
   }
   memset(pool->memory, 0, pool->size);

   pool->cellVar = readable_malloc((POOL_BITMASK_SIZE / 8) * pool->capacity);
   if (!pool->cellVar) {
      LOG_ERR("Failed to allocate memory for pool->cellVar in bitmaskPool_init()");
      goto unsuccessful;
   }
   memset(pool->cellVar, 0, (POOL_BITMASK_SIZE / 8) * pool->capacity);

   LOG_INFO("uniquePool initialized successfully");
   return pool;

unsuccessful:
   uniquePool_clear(pool);
   pool = NULL;
   return NULL;
}

void uniquePool_clear(uniquePool *pool) {
   if (!pool) {
      LOG_ERR("Invalid data in uniquePool_clear");
      return;
   }
   if (pool->memory) {
      free(pool->memory);
      pool->memory = NULL;
      LOG_WARN("Cleared pool memory");
   }
   if (pool->cellVar) {
      free(pool->cellVar);
      pool->cellVar = NULL;
      LOG_WARN("Cleared pool cell variables");
   }
   free(pool);
}

uniquePool *uniquePool_rewrite(uniquePool *pool, u_int_8 side) {
   if (!pool) {
      LOG_ERR("uniquePool_rewrite: Invalid pool data");
      return NULL;
   }

   size_t oldSize = pool->size;
   u_int_32 oldCapacity = pool->capacity;
   u_int_32 oldCellCount = pool->cellCount;

   pool->size = (side == 1) ? pool->size * 2 : pool->size / 2;
   pool->capacity = ROUND_UP(pool->size / pool->sizePerFreeFrame, POOL_BITMASK_SIZE);

   void *newMem = readable_realloc(pool->memory, pool->size, oldSize);
   if (!newMem) {
      LOG_ERR("uniquePool_rewrite: Failed to reallocate memory");
      pool->size = oldSize;
      pool->capacity = oldCapacity;
      pool->cellCount = oldCellCount;
      return NULL;
   }
   pool->memory = newMem;

   void *newCellVar = readable_realloc(pool->cellVar, (POOL_BITMASK_SIZE / 8) * pool->capacity, (POOL_BITMASK_SIZE / 8) * oldCapacity);
   if (!newCellVar) {
      LOG_ERR("uniquePool_rewrite: Failed to reallocate cellVar");
      pool->size = oldSize;
      pool->capacity = oldCapacity;
      pool->cellCount = oldCellCount;
      return NULL;
   }
   pool->cellVar = newCellVar;

   return pool;
}

s_int_32 uniquePool_add(uniquePool *pool, void *data, size_t size) {
   if (!pool || !data || size <= 0) {
      LOG_ERR("uniquePool_add: Invalid data or size");
      return 0;
   }

   u_int_32 blocksNeeded = (size + pool->sizePerFreeFrame - 1) / pool->sizePerFreeFrame;
   s_int_32 targetIndex = (size > pool->sizePerFreeFrame) ?
      bitOperations[BITMASK_MULTIPLY_BIT](pool->cellVar, pool->capacity, 1, blocksNeeded) :
      bitOperations[BITMASK_FIRST_BIT](pool->cellVar, pool->capacity, 1, 1);

   printf("TARGET INDEX: %d, size: %zu, pool size: %zu, sizePerFreeFrame: %d\n", targetIndex, size, pool->size, pool->sizePerFreeFrame);

   if (targetIndex == RESULT_TRASH) {
      LOG_ERR("uniquePool_add: No free slots available");
      return RESULT_TRASH;
   }

   void *mem_place = get_mem_place(pool, targetIndex, pool->sizePerFreeFrame, true);
   if (!mem_place || (u_int_8*)mem_place + size > (u_int_8*)pool->memory + pool->size) {
      LOG_ERR("uniquePool_add: Invalid memory place or out of bounds");
      for (u_int_32 index = 0; index < blocksNeeded; index++) {
         bitOperations[BITMASK_CURRENT_BIT](pool->cellVar, targetIndex + index, 0, 0);
      }
      return RESULT_TRASH;
   }

   memcpy(mem_place, data, size);
   pool->cellCount += blocksNeeded;
   printf("uniquePool_add: Added data at index %d, size %zu, blocks %u", targetIndex, size, blocksNeeded);
   return targetIndex;
}

s_int_32 uniquePool_remove(uniquePool *pool, s_int_32 targetIndex, size_t size) {
   if (!pool || size <= 0) {
      LOG_ERR("uniquePool_remove: Invalid data or size");
      return RESULT_TRASH;
   }

   u_int_32 blocksNeeded = (size + pool->sizePerFreeFrame - 1) / pool->sizePerFreeFrame;
   for (u_int_32 index = 0; index < blocksNeeded; index++) {
      bitOperations[BITMASK_CURRENT_BIT](pool->cellVar, targetIndex + index, 0, 0);
   }

   if (targetIndex != RESULT_TRASH) {
      void *mem_place = get_mem_place(pool, targetIndex, pool->sizePerFreeFrame, true);
      if (!mem_place || (u_int_8*)mem_place + size > (u_int_8*)pool->memory + pool->size) {
         LOG_ERR("uniquePool_remove: Invalid memory place or out of bounds");
         return RESULT_TRASH;
      }

      memset(mem_place, 0, size);
      pool->cellCount = (pool->cellCount >= blocksNeeded) ? pool->cellCount - blocksNeeded : 0;

      return targetIndex;
   }
   return RESULT_TRASH;
}
#pragma endregion

// === Node and Type Checking ===

#pragma region PART_UTILS

baseNode *getBaseNode(void *ptr) {
   return (baseNode *)ptr;
}

void setDefaultBaseNode(baseNode *node, void *parent) {
   if (!node) return;
   node->name = "";
   node->parent = parent;
   node->children = NULL;
   node->childCount = 0;
   node->kindtype = PARENT_TYPE_NULL;
   node->variables = malloc(8);
   node->variablesSize = 8;
}

void clearBaseNode(baseNode *node) {
   if (!node) return;
   if (node->variables) {
      free_readable_malloc(node->variables, node->variablesSize);
      node->variables = NULL;
      node->variablesSize = 0;
   }
}

bool checkIsTypeWorkspace(void *parent) {
   if (!parent) return false;
   baseNode *node = getBaseNode(parent);
   return (node && strcmp(node->name, "workspace") == 0);
}

kindStructType getIsKindType(void *parent) {
   if (!parent) return PARENT_TYPE_NULL;
   baseNode *node = getBaseNode(parent);
   return node->kindtype;
}

// === Child Management ===

void changeVoidChild(void ***childrenPtr, u_int_32 *childCount, void *child) {
   size_t oldSize = sizeof(void *) * (*childCount);
   size_t newSize = sizeof(void *) * (*childCount + 1);
   void **newChildren = readable_realloc(*childrenPtr, newSize, oldSize);
   if (!newChildren) return;

   *childrenPtr = newChildren;
   (*childrenPtr)[*childCount] = child;
   *childCount += 1;
}

void childAdd__(void *parent, void *child) {
   if (!parent || !child) return;
   baseNode *node = getBaseNode(parent);
   if (!node) return;
   changeVoidChild(&node->children, &node->childCount, child);
}

void removeChildrens(void ***children, u_int_32 *childCount) {
   if (!children || !*children || *childCount <= 0) return;

   for (s_int_32 index = 0; index < *childCount; index++) {
      void *child = (*children)[index];
      if (!child) {
         LOG_WARN("removeChildrens: null child");
         continue;
      }
      baseNode *node = getBaseNode(child);
      if (node->kindtype == PARENT_TYPE_OBJECT) {
         remove_object((objectSpace *)child);
      } else if (node->kindtype == PARENT_TYPE_PART) {
         remove_partStruct((partStruct *)child);
      }
   }
   free_readable_malloc(*children, (*childCount) * sizeof(void *));
   *children = NULL;
   *childCount = 0;
}

void childDestroy(void *ptr) {
   if (!ptr) return;
   baseNode *node = getBaseNode(ptr);
   if (!node) {
      LOG_WARN("childDestroy: NODE NULL!");
      return;
   }
   switch (node->kindtype) {
      case PARENT_TYPE_OBJECT:
         LOG_WARN("removing PARENT_TYPE_OBJECT");
         remove_object((objectSpace *)ptr);
         break;
      case PARENT_TYPE_PART:
         LOG_INFO("removing PARENT_TYPE_PART");
         remove_partStruct((partStruct *)ptr);
         break;
      default:
         break;
   }
}

#pragma endregion

// === Object Management ===

#pragma region OBJECT_MANAGMENT

objectSpace *instance_object(void *parent) {
   if (!parent) return NULL;

   LOG_INFO("Adding object");
   objectSpace *object = readable_malloc(sizeof(objectSpace));

   baseNode *node = getBaseNode(object);
   setDefaultBaseNode(node, parent);
   node->kindtype = PARENT_TYPE_OBJECT;

   childAdd__(parent, object);
   return object;
}

void remove_object(objectSpace *object) {
   if (!object) return;
   baseNode *node = getBaseNode(object);
   removeChildrens(&node->children, &node->childCount);
   clearBaseNode(node);
   free_readable_malloc(object, sizeof(*object));
}

// === partStruct Management ===

s_int_32 setDefaultPartStruct(partStruct *partStruct, void *parent) {
   if (!partStruct) return RESULT_TRASH;

   baseNode *node = getBaseNode(partStruct);
   setDefaultBaseNode(node, parent);
   node->kindtype = PARENT_TYPE_PART;

   partStruct->flags = 0;
   partStruct->transparency = 255;
   partStruct->size = (Vector3){2.0, 2.0, 2.0};
   partStruct->orientation = (Vector3){0.0, 0.0, 0.0};
   partStruct->material = MATERIAL_TYPE_PLASTIC;
   partStruct->brickColor = (Color){127, 127, 127, partStruct->transparency};
   partStruct->collisionGroup = COLLISION_GROUP_ONE;

   return 0;
}

bitmaskPool *getPartPoolByType(objectType type) {
   bitmaskPool *pool = NULL;
   switch (type) {
      case OBJECT_TYPE_CUBE:     pool = hierarchyBitPools.pool_partCube; break;
      case OBJECT_TYPE_SPHERE:   pool = hierarchyBitPools.pool_partSphere; break;
      case OBJECT_TYPE_CYLINDER: pool = hierarchyBitPools.pool_partCylinder; break;
      case OBJECT_TYPE_MESHPART: pool = hierarchyBitPools.pool_partMesh; break;
      default:                   break;
   }
   return pool;
}

u_int_32 instance_partStruct(partStruct *part) {
   u_int_32 targetIndex = uniquePool_add(hierarchyUniquePools.pool_partStructPool, part, sizeof(partStruct));
   return targetIndex;
}

partCube *instance_cube(void *parent, partStruct *partStruct, bitmaskPool *pool) {
   __INIT_PART__(partCube, partStruct, pool);
   partCube *createdPart = (partCube *)partStruct->shape;
   if (!createdPart) {
      return NULL;
   }

   partStruct->partType = OBJECT_TYPE_CUBE;
   LOG_INFO("partCube: created");
   return createdPart;
}

partSphere *instance_sphere(void *parent, partStruct *partStruct, bitmaskPool *pool, float radius) {
   __INIT_PART__(partSphere, partStruct, pool);
   partSphere *createdPart = (partSphere *)partStruct->shape;
   if (!createdPart) {
      return NULL;
   }

   partStruct->partType = OBJECT_TYPE_SPHERE;
   createdPart->radius = (radius == 0.0f || isnan(radius)) ? 1.0f : radius;

   LOG_INFO("partSphere: created");
   return createdPart;
}

partCylinder *instance_cylinder(void *parent, partStruct *partStruct, bitmaskPool *pool, float radiusTop, float radiusBottom, float height, u_int_8 slices) {
   __INIT_PART__(partCylinder, partStruct, pool);
   partCylinder *createdPart = (partCylinder *)partStruct->shape;
   if (!createdPart) {
      return NULL;
   }

   partStruct->partType = OBJECT_TYPE_CYLINDER;

   createdPart->radiusTop = (radiusTop == 0.0f || isnan(radiusTop)) ? 1.0f : radiusTop;
   createdPart->radiusBottom = (radiusBottom == 0.0f || isnan(radiusBottom)) ? 1.0f : radiusBottom;
   createdPart->height = (height == 0.0f || isnan(height)) ? 1.0f : height;
   createdPart->slices = (slices == 0 || (slices <= 0)) ? 1 : slices;

   LOG_INFO("partCylinder: created");
   return createdPart;
}

partMesh *instance_meshPart(void *parent, partStruct *partStruct, bitmaskPool *pool, u_int_16 mesh, u_int_16 texture, u_int_16 meshPath) {
   __INIT_PART__(partMesh, partStruct, pool);
   partMesh *createdPart = (partMesh *)partStruct->shape;
   if (!createdPart) {
      return NULL;
   }

   partStruct->partType = OBJECT_TYPE_MESHPART;

   createdPart->mesh = mesh;
   createdPart->texture = texture;
   // createdPart->meshPath = NULL;
   // changePathVar(&createdPart->meshPath, (meshPath ? meshPath : NULL));

   LOG_INFO("partMesh: created");
   return createdPart;
}

partStruct *instancePart(void *parent, objectType type, void *params) {
   void *actualParent = parent ? parent : workspace;

   partStruct tempPartStruct = {0};
   partStruct *newPartStruct = &tempPartStruct;

   u_int_32 targetIndex = instance_partStruct(&tempPartStruct);

   newPartStruct->poolIndex = targetIndex;

   if (targetIndex == RESULT_TRASH) {
      LOG_ERR("newpartStruct: cant create in instancePart");
      return NULL;
   }

   setDefaultPartStruct(&tempPartStruct, parent);
   bool success = false;

   switch (type) {
      case OBJECT_TYPE_CUBE:
         success = instance_cube(actualParent, &tempPartStruct, hierarchyBitPools.pool_partCube);
         break;
      case OBJECT_TYPE_SPHERE:
         success = instance_sphere(actualParent, &tempPartStruct, hierarchyBitPools.pool_partSphere, ((partSphere *)params)->radius);
         break;
      case OBJECT_TYPE_CYLINDER: {
                                    partCylinder *p = (partCylinder *)params;
                                    success = instance_cylinder(actualParent, &tempPartStruct, hierarchyBitPools.pool_partCylinder, 
                                          p->radiusTop, p->radiusBottom, p->height, p->slices);
                                    break;
                                 }
      case OBJECT_TYPE_MESHPART: {
                                    partMesh *p = (partMesh *)params;
                                    success = instance_meshPart(actualParent, &tempPartStruct, hierarchyBitPools.pool_partMesh, 
                                          p->mesh, p->texture, p->meshPath);
                                    break;
                                 }
      default:
                                 remove_partStruct(&tempPartStruct);
                                 return NULL;
   }
   if (!success) {
      LOG_ERR("instancePart: null yes");
      remove_partStruct(&tempPartStruct);
      return NULL;
   }
   // childAdd__(parent, newpartStruct);
   return newPartStruct;
}

#pragma endregion

// === partStruct Removal ===

#pragma region PART_REMOVE_MANAGMENT

void remove_part_general(void **part, bitmaskPool *pool, s_int_32 targetIndex) {
   bitmaskPool_remove(pool, targetIndex);
   *part = NULL;
}

void remove_cube(void **part, bitmaskPool *pool, s_int_32 targetIndex) {
   if (!pool) {
      LOG_ERR("remove_cube: pool is null");
      return;
   }
   partCube *convPart = (partCube *)part;
   if (convPart) {
      remove_part_general(part, pool, targetIndex);
   }
}

void remove_sphere(void **part, bitmaskPool *pool, s_int_32 targetIndex) {
   if (!pool) {
      LOG_ERR("remove_sphere: pool is null");
      return;
   }
   partSphere *convPart = (partSphere *)part;
   if (convPart) {
      remove_part_general(part, pool, targetIndex);
   }
}

void remove_cylinder(void **part, bitmaskPool *pool, s_int_32 targetIndex) {
   if (!pool) {
      LOG_ERR("remove_cylinder: pool is null");
      return;
   }
   partCylinder *convPart = (partCylinder *)part;
   if (convPart) {
      remove_part_general(part, pool, targetIndex);
   }
}

void remove_meshPart(void **part, bitmaskPool *pool, s_int_32 targetIndex) {
   if (!pool) {
      LOG_ERR("remove_meshPart: pool is null");
      return; 
   }
   partMesh *convPart = (partMesh *)part;
   if (convPart) {
      remove_part_general(part, pool, targetIndex);
   }
}

void removePart(partStruct *partStruct, bitmaskPool *pool) {
   if (!partStruct) return;
   if (partStruct->partType < OBJECT_TYPE_COUNT && removePartFuncs[partStruct->partType]) {
      removePartFuncs[partStruct->partType](partStruct->shape, pool, partStruct->shapePoolIndex);
   } else {
      LOG_ERR("removePart: removePartFunc is null");
   }
}

void remove_partStruct(partStruct *part) {
   if (!part) return;
   baseNode *node = getBaseNode(part);
   bitmaskPool *shapePool = getPartPoolByType(part->partType);
   if (shapePool) {
      if (part->shape) {
         removePart(part, shapePool);
         part->shape = NULL;
      }
   }

   removeChildrens(&node->children, &node->childCount);
   clearBaseNode(node);
   if (part->poolIndex >= 0) {
      uniquePool_remove(hierarchyUniquePools.pool_partStructPool, part->poolIndex, sizeof(partStruct));
      LOG_INFO("remove_partStruct: Removed partStruct");
   }
}

#pragma endregion

// === Workspace Management ===

#pragma region WORKSPACE_MANAGMENT

void init_bitPools() {
   hierarchyBitPools.pool_partCube = bitmaskPool_init(INIT_POOL_SIZE_CUBE, sizeof(partCube));
   hierarchyBitPools.pool_partSphere = bitmaskPool_init(INIT_POOL_SIZE_SPHERE, sizeof(partSphere));
   hierarchyBitPools.pool_partCylinder = bitmaskPool_init(INIT_POOL_SIZE_CYLINDER, sizeof(partCylinder));
   hierarchyBitPools.pool_partMesh = bitmaskPool_init(INIT_POOL_SIZE_MESHPART, sizeof(partMesh));
   if (!hierarchyBitPools.pool_partCube || !hierarchyBitPools.pool_partSphere ||
         !hierarchyBitPools.pool_partCylinder || !hierarchyBitPools.pool_partMesh) {
      LOG_ERR("Failed to initialize bitmask pools");
   }
}

void init_uniquePools() {
   hierarchyUniquePools.pool_partStructPool = uniquePool_init(INIT_PART_STRUCT_POOL_SIZE, sizeof(partStruct)); 
}

void init_pools() {
   init_bitPools();
}

void init_workspace() {
   remove_workspace();

   init_bitPools();
   init_uniquePools();

   srand(time(NULL));
   workspace = readable_malloc(sizeof(objectSpace));
   baseNode node = workspace->mynode;
   setDefaultBaseNode(&node, NULL);
   node.kindtype = PARENT_TYPE_OBJECT;
   node.name = "workspace";

   for (s_int_32 i = 0; i < 69; i++) {
      partStruct *part = instancePart(workspace, OBJECT_TYPE_MESHPART, INIT_DUMMY(partMesh));
      printf("SIZE: : : %zu\n", sizeof(*part));
      if (part != NULL && part->poolIndex >= 0) {
         remove_partStruct(part);
      }
   }

   LOG_INFO("Initializing workspace");
}

void remove_workspace() {
   if (workspace) {
      free_readable_malloc(workspace, sizeof(*workspace));
      workspace = NULL;
   }
}

#pragma endregion
