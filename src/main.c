#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "settings.h"

#define CRASH_BLUE (Color){16, 10, 209, 255}
#define CYAN (Color){0, 255, 255, 255}
#define WALL_COLOUR (Color){90, 150, 255, 255}

#define MUSIC_VOLUME 0.6f
#define MUSIC_VOLUME_QUIET 0.3f

#define RESTART_MESSAGE_LENGTH 350
#define UPGRADES 3
#define UPGRADE_LEVELS 8

#define WINDOW_CENTRE (Vector2){WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f}

typedef enum {
  STATE_GAME,
  STATE_UPGRADE,
  STATE_RELOAD,
  STATE_GAMEOVER,
} GameStates;

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "game");
  InitAudioDevice();
  SetAudioStreamBufferSizeDefault(8192);
  HideCursor();
  SetTargetFPS(FPS);

  // Set to zero for easy bug reproducability.
  SetRandomSeed(0);

  Music music = LoadMusicStream("assets/Tron Legacy - Son of Flynn (Remix).ogg");
  SetMusicVolume(music, MUSIC_VOLUME);
  PlayMusicStream(music);

  Sound virus_sfx = LoadSound("assets/virus.ogg");
  Sound static_sfx = LoadSound("assets/static.ogg");

  Font font_c64 = LoadFont("assets/C64_Pro-STYLE.ttf");
  Font font_opensans = LoadFontEx("assets/OpenSans-Light.ttf", 256, NULL, 255);

  Camera camera3D = {{0.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, CAMERA_PERSPECTIVE};
  camera3D.fovy = 10.0f;
  camera3D.projection = CAMERA_ORTHOGRAPHIC;

  Shader shader_scanlines = LoadShader(0, TextFormat("assets/shaders%i/scanlines.fs", GLSL_VERSION));
  Shader shader_blur = LoadShader(0, TextFormat("assets/shaders%i/blur.fs", GLSL_VERSION));
  Shader shader_glitch = LoadShader(0, TextFormat("assets/shaders%i/glitch.fs", GLSL_VERSION));
  int timeLoc = GetShaderLocation(shader_glitch, "time");

  RenderTexture2D target_entities = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
  RenderTexture2D target_world = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

  Camera2D camera = {};
  camera.target = Vector2Zero();
  camera.offset = Vector2Zero();
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  char bugs_collected_text[50];
  char upgrades_text[100];
  char upgrades_price_text[50];

  char restart_message_target[RESTART_MESSAGE_LENGTH];
  char restart_message_live[RESTART_MESSAGE_LENGTH];
  int char_index = 0;
  int delay_left = 0;

  int upgrade_index = 0;
  int upgrade_level[UPGRADES] = {0};
  int upgrade_price[UPGRADE_LEVELS] = {8, 16, 32, 128, 256, 1024, 4096, 16384};

  float time;

  while (!WindowShouldClose()) {
    time = (float)GetTime();
    SetShaderValue(shader_glitch, timeLoc, &time, SHADER_UNIFORM_FLOAT);
    UpdateMusicStream(music);
    SetMusicVolume(music, MUSIC_VOLUME);

    int level = upgrade_level[upgrade_index];
    if (level < UPGRADE_LEVELS) {
      int price = upgrade_price[level];
      sprintf(upgrades_price_text, "%d BUGS", price);
    } else {
      sprintf(upgrades_price_text, "CANNOT UPGRADE");
    }
    int max_width = 10;
    int max_height = 3;
    int max_time = 100;
    

    sprintf(upgrades_text, "WIDTH: %d\t\t[%d/%d]\n\n\nHEIGHT: %d\t\t[%d/%d]\n\n\nTIME: %d\t\t[%d/%d]", (int)max_width, upgrade_level[0],
                  UPGRADE_LEVELS, (int)max_height, upgrade_level[1], UPGRADE_LEVELS, max_time, upgrade_level[2], UPGRADE_LEVELS);
    sprintf(bugs_collected_text, "BUGS COLLECTED: %d", 0);

    Rectangle selection_rect = (Rectangle){10, upgrade_index * 77 + 290, 440, 40};

    BeginTextureMode(target_world);
      ClearBackground(BLACK);
      DrawTextEx(font_c64, "DEAD PIXEL", (Vector2){10, 10}, 80, 1, WHITE);
      DrawTextEx(font_c64,
                 "CONTROLS:\n\n\n"
                 "<WASD> To move and stretch\n\n"
                 "<J> To jump and select\n\n"
                 "<K> (HOLD) To stretch\n\n"
                 "(TAP 2x) To shrink\n\n\n\n"
                 "<R> To manually restart\n\n"
                 "<SPACE> To start\n\n\n\n\n\n\n"
                 "SEBZANARDO 2025",
                 (Vector2){830, 300}, 16, 1, GRAY);
      DrawTextEx(font_c64, upgrades_text, (Vector2){30, 300}, 24, 1, YELLOW);
      DrawTextEx(font_c64, bugs_collected_text, (Vector2){30, 550}, 16, 1, GREEN);
      DrawRectangleRoundedLinesEx(selection_rect, 0.1f, 4.0f, 4.0f, MAGENTA);
      DrawRectangle(25, upgrade_index * 77 + 280, 220, 20, BLACK);
      DrawTextEx(font_c64, upgrades_price_text, (Vector2){30, upgrade_index * 77 + 280}, 16, 1, GREEN);
    EndTextureMode();

    BeginDrawing();
      BeginShaderMode(shader_blur);
      DrawTextureRec(target_world.texture,
                     (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height},
                     (Vector2){0, 0}, WHITE);
      EndShaderMode();
      BeginShaderMode(shader_scanlines);
      DrawTextureRec(target_entities.texture,
                     (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height},
                     (Vector2){0, 0}, WHITE);
      EndShaderMode();
    EndDrawing();
  }

  // De-Initialization
  UnloadShader(shader_scanlines);
  UnloadRenderTexture(target_entities);
  UnloadMusicStream(music);
  CloseAudioDevice();

  CloseWindow();
}
