#include "raylib.h"
#include "raymath.h"


#define GLSL_VERSION 330


const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const Vector2 WINDOW_CENTRE = (Vector2) {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
const int FPS = 60;
const float FIXED_DT = 1.0f / FPS;


int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    SetTargetFPS(FPS);

    Camera2D camera = {};
    camera.target = Vector2Zero();
    camera.offset = Vector2Zero();
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Shader shader = LoadShader(0, TextFormat("src/resources/scanlines.fs", GLSL_VERSION));
    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    /*int scanline_offset = GetShaderLocation(shader, "offset");*/
    /*float elapsed_time = 0.0f;*/

    while (!WindowShouldClose()) {
        /*elapsed_time += FIXED_DT;*/
        /*SetShaderValue(shader, scanline_offset, &elapsed_time, SHADER_UNIFORM_FLOAT);*/

        // Render to a texture for textures affected by postprocessing shaders
        BeginTextureMode(target);
            BeginMode2D(camera);
                ClearBackground(BLACK);
                DrawCircleV(WINDOW_CENTRE, 50.0f, MAGENTA);
            EndMode2D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawTexture(target.texture, 0, 0, BLACK);
            EndShaderMode();
            DrawFPS(0, 0);
        EndDrawing();
    }

    // De-Initialization
    UnloadShader(shader);
    UnloadRenderTexture(target);

    CloseWindow();
}
