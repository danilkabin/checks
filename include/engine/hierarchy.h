#ifndef HIERARCHY_H
#define HIERARCHY_H

#include "raylib.h"
#include "typedef.h"
#include <stdbool.h>

// ENUM TYPEDEFS
typedef enum {
   OBJECT_TYPE_PART,
   OBJECT_TYPE_SPHERE,
   OBJECT_TYPE_CYLINDER,
   OBJECT_TYPE_GRID,
   OBJECT_TYPE_MESHPART,
   OBJECT_TYPE_FOLDER
} objectType;

typedef enum {
   PARENT_TYPE_OBJECT,
   PARENT_TYPE_WORKSPACE
} parentType;

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

// STRUCT TYPEDEFS
// // GENERAL DEFINITIONS
typedef struct {
   float transparency;
   bool castShadow;
   bool anchored;

   void *parent;
   void *variables;

   Color brickColor;
   materialType material;

   u_int_32 collisionGroup;

   Vector3 position;
   Vector3 orientation;
   Vector3 size;
   Matrix orientationMatrix;
} shapeVariables;

typedef struct {
   char *name;

   void *parent;
   parentType parentKind;

   void **children;
   int childCount;

   void *partData;
   void *variables;
} objectSpace;

// // PART DEFINITIONS
typedef struct {
   shapeVariables *shape;
   
} partCube;

typedef struct {
   shapeVariables *shape;
   float radius;
} partSphere;

typedef struct {
   shapeVariables *shape;
   float radiusTop;
   float radiusBottom;
   float height;
   int slices;
} partCylinder;

// END STRUCT TYPEDEFS

extern objectSpace *workspace;

objectSpace *instance_object(void *parent);
shapeVariables *instance_shape();

partCube *instance_cube(void *parent);
partSphere *instance_sphere(void *parent, float radius);
partCylinder *instance_cylinder(void *parent, float radiusTop, float radiusBottom, float height, int slices);

int setDefaultShape(shapeVariables *shape, void *parent);

void init_workspace();
void remove_workspace();

#endif
