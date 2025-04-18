#include "engine/hierarchy.h"
#include "raylib.h"
#include "typedef.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

objectSpace *workspace;

//
// PART DEFINITIONS
//
partCube *instance_cube(void *parent) {
   void *currentParent = parent ? parent : workspace;

   partCube *part = malloc(sizeof(partCube));
   if (!part) {
      return NULL;
   }
   part->shape = instance_shape();
   if (!part->shape) {
      free(part);
      return NULL;
   }

   setDefaultShape(part->shape, currentParent);
   return part;
}

partSphere *instance_sphere(void *parent, float radius) {
   void *currentParent = parent ? parent : workspace;
   float currentRadius = radius;

   if (isnan(radius)) {
      currentRadius = 1.0;
   }

   partSphere *part = malloc(sizeof(partSphere));
   if (!part) {
      return NULL;
   }
   part->shape = instance_shape();
   if (!part->shape) {
      free(part);
      return NULL;
   } 

   part->radius = currentRadius;
   setDefaultShape(part->shape, currentParent);

   return part;
}

partCylinder *instance_cylinder(void *parent, float radiusTop, float radiusBottom, float height, int slices) {
   void *currentParent = parent ? parent : workspace;
   float currentRadiusTop = radiusTop;
   float currentRadiusBottom = radiusBottom;
   float currentHeight = height;
   int currentSlices = slices;

   if (isnan(currentRadiusTop)) currentRadiusTop = 1.0;
   if (isnan(currentRadiusBottom)) currentRadiusBottom = 1.0;
   if (isnan(currentHeight)) currentHeight = 1.0;
   if (currentSlices <= 0) currentSlices = 1;

   partCylinder *part = malloc(sizeof(partCylinder));
   if (!part) {
      return NULL;
   }
   part->shape = instance_shape();
   if (!part->shape) {
      free(part);
      return NULL;
   } 

   part->radiusTop = currentRadiusTop;
   part->radiusBottom = currentRadiusBottom;
   part->height = currentHeight;
   part->slices = currentSlices;

   setDefaultShape(part->shape, currentParent);
   return part;
}
//
// END PART DEFINITIONS
//
shapeVariables *instance_shape() {
   shapeVariables *newShape = malloc(sizeof(shapeVariables));
   return newShape;
}

objectSpace *instance_object(void *parent) {
   if (!parent) return NULL;
   LOG_INFO("Adding object");
   objectSpace *object = calloc(1, sizeof(objectSpace));
   return object;
}

int setDefaultShape(shapeVariables *shape, void *parent) {
   if (!shape) {
      return -1;
   }
   shape->anchored = false;
   shape->castShadow = true;
   shape->transparency = 0;
   shape->size = (Vector3){2.0, 2.0, 2.0};
   shape->orientation = (Vector3){0.0, 0.0, 0.0};
   shape->material = MATERIAL_TYPE_PLASTIC;
   shape->brickColor = (Color){127, 127, 127, 255};
   shape->collisionGroup = COLLISION_GROUP_ONE;
   shape->parent = parent;

   return 0;
}

void init_workspace() {
   remove_workspace();
   workspace = malloc(sizeof(objectSpace));
   workspace->name = "workspace";
   workspace->parent = NULL;
   workspace->parentKind = PARENT_TYPE_WORKSPACE;
   LOG_INFO("Initializing workspace");
}

void remove_workspace() {
   if (workspace) {

      free(workspace);
      workspace = NULL;
   }
}
