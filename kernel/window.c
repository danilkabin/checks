#include "window.h"
#include <raylib.h>

int ebaloWindow() {
   InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, PROJECT_NAME);
   return 0;
}

int killWindow() {
   CloseWindow();
   return 0;
}
