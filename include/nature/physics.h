#ifndef PHYSICS_H
#define PHYSICS_H

#include <raylib.h>

typedef enum {
   PART_MESH,
   PART_SPHERE,
   PART_PARTICLE,
   PART_CUSTOM_LOGIC
} PartType;

typedef struct {
   void *parent;
   void *parts;
   void *variables;

   char *name;
   bool anchored;
   bool canCollide;
   float transparency; 

   Vector3 position;
   Vector3 orientation;
   Vector3 scale;


} objectModel;

#endif
