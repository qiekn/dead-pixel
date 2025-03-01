#include "raylib.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int FPS = 60;
const float FIXED_DT = 1.0f / FPS;

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    SetTargetFPS(FPS);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(0, 0);
        EndDrawing();
    }

    CloseWindow();
}
