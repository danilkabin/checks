#include "playerModule/player.h"
#include "cameraModule/camera.h"
#include "planeModule/airplane.h"
#include "typedef.h"
#include "engine/hierarchy.h"

planeShape *currentPlane;

void playerAdded() {
   init_workspace();
   instance_object(workspace);
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
