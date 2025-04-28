#ifndef HIERARCHY_H
#define HIERARCHY_H

#include "raylib.h"
#include "typedef.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_POOL_SIZE 256
#define FLAG_ANCHORED (1 << 0)
#define FLAG_CANCOLLIDE (1 << 1)

#define POOL_BITMASK_SIZE 64

#define ROUND_UP(first, second) (((first) + (second) - 1) / (second))
#define IS_BIT_SET(val, idx) (((val) >> (idx)) & 1UL)

#define INIT_DUMMY(T) ((T[]){{0}})

#define __INIT_PART__(TYPE, partStruct, POOL) \
   TYPE part = {0}; \
   partPoolInit_struct res = initPartPool(POOL, &part, sizeof(TYPE)); \
   if (!res.part) { \
      return NULL; \
   } \
   TYPE *partPtr = (TYPE*)res.part; \
   partStruct->poolIndex = res.targetIndex; \
   partStruct->shape = partPtr; \

#define INIT_POOL_SIZE_CUBE 1024
#define INIT_POOL_SIZE_SPHERE 1024
#define INIT_POOL_SIZE_CYLINDER 1024
#define INIT_POOL_SIZE_MESHPART 1024

#define INIT_PART_STRUCT_POOL_SIZE 2048

#define RESULT_TRASH -228

typedef enum u_int_8 {
   OBJECT_TYPE_CUBE,
   OBJECT_TYPE_SPHERE,
   OBJECT_TYPE_CYLINDER,
   OBJECT_TYPE_MESHPART,
   OBJECT_TYPE_COUNT
} objectType;

typedef enum {
   POOL_TYPE_FIXED,
   POOL_TYPE_DYNAMIC
} poolType;

typedef enum {
   BITMASK_FIRST_BIT,
   BITMASK_CURRENT_BIT,
   BITMASK_MULTIPLY_BIT,
   BITMASK_COUNT
} bitmaskType;

typedef enum {
   PARENT_TYPE_OBJECT,
   PARENT_TYPE_PART,
   PARENT_TYPE_NULL
} kindStructType;

typedef enum {
   MATERIAL_TYPE_PLASTIC,
   MATERIAL_TYPE_BRICK
} materialType;

typedef enum {
   COLLISION_GROUP_ONE = 1 << 0,
   COLLISION_GROUP_TWO = 1 << 1,
   COLLISION_GROUP_THREE = 1 << 2,
   COLLISION_GROUP_FOUR = 1 << 3,
   COLLISION_GROUP_FIVE = 1 << 4,
   COLLISION_GROUP_SIX = 1 << 5,
   COLLISION_GROUP_SEVEN = 1 << 6,
   COLLISION_GROUP_EIGHT = 1 << 7,
   COLLISION_GROUP_NINE = 1 << 8,
   COLLISION_GROUP_TEN = 1 << 9,
   COLLISION_GROUP_ELEVEN = 1 << 10,
   COLLISION_GROUP_TWELVE = 1 << 11
} collisionGroup;

typedef struct {
   kindStructType kindtype;
   char *name;
   void *parent;
   void **children;
   u_int_32 childCount;
   u_int_32 childCapacity;
   void *variables;
   s_int_32 variablesSize;
} baseNode;

typedef struct {
   baseNode mynode;
   u_int_8 transparency;
   u_int_8 flags;
   u_int_32 poolIndex;
   u_int_32 shapePoolIndex;
   void *shape;
   objectType partType;
   Color brickColor;
   u_int_8 material;
   u_int_16 collisionGroup;
   Vector3 position;
   Vector3 orientation;
   Vector3 size;
} partStruct;

typedef struct {
   baseNode mynode;
   void *partData;
} objectSpace;

typedef struct {
} workspaceObject;

typedef struct {
   char live;
} partCube;

typedef struct {
float radius;
} partSphere;

typedef struct {
   float radiusTop;
   float radiusBottom;
   float height;
   u_int_8 slices;
} partCylinder;

typedef struct {
   u_int_16 mesh;
   u_int_16 texture;
   u_int_16 meshPath;
} partMesh;

typedef struct {
   void *memory;
   size_t size;
   bool isActive;
   u_int_64 *cellVar;
   u_int_32 cellCount;
   u_int_32 sizePerFrame;
   u_int_32 capacity;
} bitmaskPool;

typedef struct {
   void *memory;
   size_t size;
   u_int_16 sizePerFreeFrame;
   u_int_32 cellCount;
   u_int_64 *cellVar;
   u_int_32 capacity;
} uniquePool;

extern objectSpace *workspace;

void *readable_malloc(size_t len);
void free_readable_malloc(void *ptr, size_t len);

objectSpace *instance_object(void *parent);
void remove_object(objectSpace *object);
void removeChildrens(void ***children, u_int_32 *childCount);

u_int_32 instance_partStruct(partStruct *part);

partCube *instance_cube(void *parent, partStruct *partStruct, bitmaskPool *pool);
partSphere *instance_sphere(void *parent, partStruct *partStruct, bitmaskPool *pool, float radius);
partCylinder *instance_cylinder(void *parent, partStruct *partStruct, bitmaskPool *pool, float radiusTop, float radiusBottom, float height, u_int_8 slices);
partMesh *instance_meshPart(void *parent, partStruct *partStruct, bitmaskPool *pool, u_int_16 mesh, u_int_16 texture, u_int_16 meshPath);

baseNode *getBaseNode(void *ptr);
void setDefaultBaseNode(baseNode *node, void *parent);

void remove_partStruct(partStruct *partStruct);

void remove_cube(void **part, bitmaskPool *pool, s_int_32 targetIndex);
void remove_sphere(void **part, bitmaskPool *pool, s_int_32 targetIndex);
void remove_cylinder(void **part, bitmaskPool *pool, s_int_32 targetIndex);
void remove_meshPart(void **part, bitmaskPool *pool, s_int_32 targetIndex);

partStruct *instancePart(void *parent, objectType type, void *params);

void removePart(partStruct *partStruct, bitmaskPool *pool);

void childDestroy(void *ptr);

s_int_32 setDefaultPartStruct(partStruct *partStruct, void *parent);

void removePoolChild();

bool checkIsTypeWorkspace(void *parent);
void remove_workspace();

void childAdd__(void *parent, void *children);
void childRemove__();

void init_workspace();

s_int_32 changeFirstBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable);
s_int_32 changeCurrentBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable);
s_int_32 changeMultiplyBit(void *data, s_int_32 index, u_int_8 side, u_int_8 variable);

// BITMASK POOL
bitmaskPool *bitmaskPool_init(size_t size, size_t sizePerFrame);
void bitmaskPool_clear(bitmaskPool *pool);
bitmaskPool *bitmaskPool_rewrite(bitmaskPool *pool, u_int_8 side);
s_int_32 bitmaskPool_add(bitmaskPool *pool, void *data, size_t size);
void bitmaskPool_remove(bitmaskPool *pool, u_int_32 index);

uniquePool *uniquePool_init(size_t size, size_t sizePerFrame);
void uniquePool_clear(uniquePool *pool);

s_int_32 uniquePool_add(uniquePool *pool, void *data, size_t size);

void *init_pool(poolType *type, size_t size, size_t sizePerFreeFrame);

#endif
