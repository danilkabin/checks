#include "playerModule/player.h"
#include "cameraModule/camera.h"
#include "planeModule/airplane.h"
#include "typedef.h"
#include "engine/hierarchy.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>

planeShape *currentPlane;

void playerAdded() {
   init_workspace();
      partMesh meshParams = {
         .meshPath = "pudoeofe"
      };
   objectSpace *object = instance_object(workspace);
   for (int aye = 0; aye < 3; aye++) {
      shapeProperties *yes = instancePart(object, OBJECT_TYPE_FOLDER, &meshParams);
     // partMesh *partMesh = yes->part;
   }
childDestroy(object);
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
