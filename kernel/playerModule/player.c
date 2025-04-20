#include "playerModule/player.h"
#include "cameraModule/camera.h"
#include "planeModule/airplane.h"
#include "typedef.h"
#include "engine/hierarchy.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <string.h>

planeShape *currentPlane;

void playerAdded() {
   memoryPool *pool = readable_malloc(MAX_POOL_SIZE);
   init_workspace(pool);
   partMesh meshParams = {
      .meshPath = "hello"
   };

         shapeProperties *shape1 = instancePart(workspace, OBJECT_TYPE_CUBE, NULL);
         remove_shape(shape1);
   //objectSpace *obj = instance_object(workspace);

   /*
   for (int i = 0; i < 1; i++) {
      objectSpace *objyes = instance_object(obj);
      for (int i2 = 0; i2 < 3; i2++) {
         shapeProperties *shape = instancePart(objyes, OBJECT_TYPE_CUBE, NULL);
      }
   remove_object(objyes);
   }
   remove_object(obj);
*/
}

void *characterAdded() {
   planeShape *plane = spawnPlane("plane");
   if (plane == NULL) {
      LOG_WARN("characterAdded(): plane NULL");
      return NULL;
   }
   currentPlane = plane;
   return 0;
}

void runService() {
   monitorCameraState();
}
