#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>

#define DEFAULT_FOV 120

typedef enum {
   CAMERA_TARGET_FREE,
   CAMERA_TARGET_PLAYER,
   CAMERA_TARGET_DIED
} cameraStateList;


Camera3D *initCurrentCamera();
void freeCurrentCamera();

typedef struct cameraControl{
   Camera3D *currentCamera;
   cameraStateList cameraState;
   void(*updateCamera)(struct cameraControl *this);

} cameraControl;

extern cameraControl cameraController;

void monitorCameraState();


#endif
