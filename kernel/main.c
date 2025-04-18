#include "cameraModule/camera.h"
#include "planeModule/airplane.h"
#include "playerModule/player.h"
#include "raylib.h"
#include "window.h"
#include "typedef.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
   ebaloWindow();
   SetTargetFPS(60);
   
   initCurrentCamera();
   playerAdded();
   while (!WindowShouldClose()) {
      runService();
   }

   CloseWindow();
   return 0;
}
