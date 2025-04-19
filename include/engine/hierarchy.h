#ifndef HIERARCHY_H
#define HIERARCHY_H

#include "raylib.h"
#include "typedef.h"
#include <stdbool.h>
#include <stddef.h>

// ENUM TYPEDEFS
typedef enum {
   OBJECT_TYPE_CUBE,
   OBJECT_TYPE_SPHERE,
   OBJECT_TYPE_CYLINDER,
   OBJECT_TYPE_GRID,
   OBJECT_TYPE_MESHPART,
   OBJECT_TYPE_FOLDER
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

// END ENUM TYPEDEFS

typedef struct {
   kindStructType kindtype; 
   char *name;
   void *parent;
   void **children;
   int childCount;
   void *variables;
} baseNode;

// STRUCT TYPEDEFS
// // GENERAL DEFINITIONS
typedef struct {
   baseNode mynode;

   float transparency;
   bool castShadow;
   bool anchored;

   void *part;
   objectType partType; 

   Color brickColor;
   materialType material;

   u_int_32 collisionGroup;

   Vector3 position;
   Vector3 orientation;
   Vector3 size;
} shapeProperties;

typedef struct {
   baseNode mynode;

   void *partData;
} objectSpace;

// // PART DEFINITIONS
typedef struct {
   
} partCube;

typedef struct {
   float radius;
} partSphere;

typedef struct {
   float radiusTop;
   float radiusBottom;
   float height;
   int slices;
} partCylinder;

typedef struct {
   Mesh mesh;
   Texture2D texture;
   char *meshPath;
} partMesh;

// END STRUCT TYPEDEFS

extern objectSpace *workspace;

void *readable_malloc(size_t len);
void free_readable_malloc(void *ptr, size_t len);

objectSpace *instance_object(void *parent);
void remove_object(objectSpace *object);

shapeProperties *instance_shape();

partCube *instance_cube(void *parent, shapeProperties *shape);
partSphere *instance_sphere(void *parent, shapeProperties *shape, float radius);
partCylinder *instance_cylinder(void *parent, shapeProperties *shape, float radiusTop, float radiusBottom, float height, int slices);
partMesh *instance_meshPart(void *parent, shapeProperties *shape, Mesh mesh, Texture2D texture, char *meshPath);

baseNode *getBaseNode(void *ptr);
void setDefaultBaseNode(baseNode *node, void *parent);

void remove_shape(shapeProperties *shape);

void remove_cube(partCube **part);
void remove_sphere(partSphere **part);
void remove_cylinder(partCylinder **part);
void remove_meshPart(partMesh **part);

shapeProperties *instancePart(void *parent, objectType type, void *params);
void removePart(void *part, objectType type);

void childDestroy(void *ptr);

int setDefaultShape(shapeProperties *shape, void *parent);

bool checkIsTypeWorkspace(void *parent);
void init_workspace();
void remove_workspace();

void childAdd__(void *parent, void *children);
void childRemove__();

#endif
