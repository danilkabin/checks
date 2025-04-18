#ifndef PLANE_H
#define PLANE_H

#include "raylib.h"
#include "typedef.h"

typedef struct {
   Model *object;
   char *name;
   bool isDead;

   struct {
      float direction;



      u_int_32 maxSpeed;
      u_int_8 currentSpeed;
   } moveStats;

   struct {
      float maxHealth;
      float currentHealth;
      float fuel;
   } currentStats;

} planeShape;

extern planeShape *currentPlane;

planeShape *spawnPlane(char *planeType);
u_int_32 *destroyPlane(planeShape *plane);

char *makePlanePath(char *planeType);

#endif
