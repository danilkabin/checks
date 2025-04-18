
#include "cameraModule/camera.h"
#include "typedef.h"
#include <raylib.h>
#include <stdlib.h>

cameraControl cameraController;

void updateFreeCamera(cameraControl *camera) {
  UpdateCamera(camera->currentCamera, CAMERA_FREE);
}

void monitorCameraState() {
   updateFreeCamera(&cameraController);

     BeginDrawing();
     ClearBackground(RAYWHITE);
     BeginMode3D(*cameraController.currentCamera);
     DrawCube((Vector3){0.0f, 0.0f, 0.0f}, 100, 1, 100, GRAY);
     EndMode3D();
  EndDrawing();
}

Camera3D *initCurrentCamera() {
   freeCurrentCamera();
   cameraController.currentCamera = malloc(sizeof(Camera3D));
   
   *cameraController.currentCamera = (Camera3D){
      .position= {0.0f, 3.0f, 1.0f},
      .target = {0.0f, 0.0f, 0.0f},
      .up = {0.0f, 1.0f, 0.0f},
      .fovy = DEFAULT_FOV,
      .projection = CAMERA_PERSPECTIVE
   };

   return cameraController.currentCamera;
};

void freeCurrentCamera() {
   if (cameraController.currentCamera) {
      free(cameraController.currentCamera);
      cameraController.currentCamera = NULL;
   }
}
