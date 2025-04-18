#include "planeModule/airplane.h"
#include "typedef.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

char *makePlanePath(char *planeType) {
   char *modelPath = (char*)malloc(PLANE_PATH_SIZE);
   if (!modelPath) {
      return NULL;
   }
   snprintf(modelPath, PLANE_PATH_SIZE, "include/models/%s.obj", planeType);
   FILE *planeFile = fopen(modelPath, "r");
   if (planeFile) {
      fclose(planeFile);
      return modelPath;
   } else {
      free(modelPath);
      return NULL;
   }
};

planeShape *spawnPlane(char *planeType) {
   char *modelPath = makePlanePath(planeType); 
   planeShape *planeReal = (planeShape*)malloc(sizeof(planeShape));

   if (modelPath == NULL) {
      return NULL;
   }

   planeReal->object = malloc(sizeof(Model));
   *planeReal->object = LoadModel(modelPath);
   return planeReal;
};

u_int_32 *destroyPlane(planeShape *plane) {
   if (plane->object) {
      UnloadModel(*plane->object);
      free(plane->object);
      plane->object = NULL;
   }

   free(plane);
   return 0;
} 
