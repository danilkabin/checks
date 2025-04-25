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

#define __INIT_PART__(TYPE, SHAPE, POOL) \
   TYPE part = {0}; \
   partPoolInit_struct res = initPartPool(POOL, &part, sizeof(TYPE)); \
   if (!res.part) { \
      return NULL; \
   } \
   TYPE *partPtr = (TYPE*)res.part; \
   shape->poolIndex = res.targetIndex; \
   shape->part = partPtr; \

typedef enum u_int_8 {
   OBJECT_TYPE_CUBE,
   OBJECT_TYPE_SPHERE,
   OBJECT_TYPE_CYLINDER,
   OBJECT_TYPE_MESHPART,
   OBJECT_TYPE_COUNT
} objectType;

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
   int variablesSize;
} baseNode;

typedef struct {
   baseNode mynode;
   u_int_8 transparency;
   u_int_8 flags;
   u_int_32 poolIndex;
   void *part;
   objectType partType;
   Color brickColor;
   u_int_8 material;
   u_int_16 collisionGroup;
   Vector3 position;
   Vector3 orientation;
   Vector3 size;
} shapeProperties;

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
   u_int_16 cellCount;
   u_int_32 sizePerFrame;
   u_int_32 capacity;
} bitmaskPool;

extern objectSpace *workspace;

void *readable_malloc(size_t len);
void free_readable_malloc(void *ptr, size_t len);

objectSpace *instance_object(void *parent);
void remove_object(objectSpace *object);
void removeChildrens(void ***children, u_int_32 *childCount);

shapeProperties *instance_shape();

partCube *instance_cube(void *parent, shapeProperties *shape, bitmaskPool *pool);
partSphere *instance_sphere(void *parent, shapeProperties *shape, bitmaskPool *pool, float radius);
partCylinder *instance_cylinder(void *parent, shapeProperties *shape, bitmaskPool *pool, float radiusTop, float radiusBottom, float height, u_int_8 slices);
partMesh *instance_meshPart(void *parent, shapeProperties *shape, bitmaskPool *pool, u_int_16 mesh, u_int_16 texture, u_int_16 meshPath);

baseNode *getBaseNode(void *ptr);
void setDefaultBaseNode(baseNode *node, void *parent);

void remove_shape(shapeProperties *shape);

void remove_cube(void **part);
void remove_sphere(void **part);
void remove_cylinder(void **part);
void remove_meshPart(void **part);

shapeProperties *instancePart(void *parent, objectType type, void *params);
void removePart(shapeProperties *shape, bitmaskPool *pool);

void childDestroy(void *ptr);

int setDefaultShape(shapeProperties *shape, void *parent);

void removePoolChild();

bool checkIsTypeWorkspace(void *parent);
void remove_workspace();

void childAdd__(void *parent, void *children);
void childRemove__();

void init_workspace();

// BITMASK
bitmaskPool *bitmaskPool_init(size_t size, size_t sizePerFrame);
void bitmaskPool_clear(bitmaskPool *pool);
bitmaskPool *bitmaskPool_rewrite(bitmaskPool *pool, u_int_8 side);
u_int_32 bitmaskPool_add(bitmaskPool *pool, void *data, size_t size);
void bitmaskPool_remove(bitmaskPool *pool, u_int_32 index);

#endif
