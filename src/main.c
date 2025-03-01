#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdio.h>


#define GLSL_VERSION 330

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FPS 120
#define FIXED_DT (1.0f / FPS)

#define CELL_SIZE 16
#define LEVEL_WIDTH (WINDOW_WIDTH / CELL_SIZE)
#define LEVEL_HEIGHT (WINDOW_HEIGHT / CELL_SIZE)

#define MAX_KEYBINDS 4

const Vector2 WINDOW_CENTRE = (Vector2) {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
const float SMOL = 0.01f;
const float MIN_SIZE = CELL_SIZE - 1.0f;


typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    UP,
} Action;

typedef enum {
    EMPTY,
    SOLID,
} CellType;

typedef struct {
    int keybinds[MAX_KEYBINDS];
    Rectangle aabb;
    Vector2 vel;
    Vector2 max_vel;
    Vector2 min_vel;
    float max_size;
    float speed;
    float friction;
    float gravity;
    float min_jump_speed;
    float max_jump_speed;
    float fall_multiplier;
    float coyote_time;
    float coyote_time_left;
    float jump_buffer;
    float jump_buffer_left;
    bool is_grounded;
} Player;


bool inside_level(int x, int y);


int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    HideCursor();
    SetTargetFPS(FPS);

    Camera2D camera = {};
    camera.target = Vector2Zero();
    camera.offset = Vector2Zero();
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Shader shader = LoadShader(0, TextFormat("src/resources/scanlines.fs", GLSL_VERSION));
    /*int scanline_offset = GetShaderLocation(shader, "offset");*/
    /*float elapsed_time = 0.0f;*/

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Load level from file
    FILE* file_ptr = fopen("src/resources/level.txt", "r");

    if (file_ptr == NULL) {
        printf("ERROR: Could not open file!\n");
        return 1;
    }

    int level[LEVEL_HEIGHT][LEVEL_WIDTH] = {0};

    // TODO: Make this better. Load file into array and resize array???
    int x = 0;
    int y = 0;
    int cell_type;
    while (fscanf(file_ptr, "%d", &cell_type) == 1 && y < LEVEL_HEIGHT) {
        level[y][x] = cell_type;
        x++;
        if (fgetc(file_ptr) == '\n' || x >= LEVEL_WIDTH) {
            x = 0;
            y++;
        }
    }

    fclose(file_ptr);

    Player player = {0};
    player.keybinds[RIGHT] = 262;
    player.keybinds[LEFT] = 263;
    player.keybinds[DOWN] = 264;
    player.keybinds[UP] = 265;
    player.aabb = (Rectangle){
        WINDOW_CENTRE.x, WINDOW_CENTRE.y,
        CELL_SIZE - 1.0f, CELL_SIZE - 1.0f
    };
    player.vel = Vector2Zero();
    player.max_vel = (Vector2){200.0f, 400.0f};
    player.min_vel = Vector2Negate(player.max_vel);
    player.max_size = CELL_SIZE * 8.0f - 1.0f;
    player.speed = 600.0f;
    player.friction = 0.0001f;  // Between 0 - 1, higher means lower friction
    float min_jump = CELL_SIZE * 1.5f;
    float max_jump = CELL_SIZE * 4.5f;
    float time_to_jump_apex = 0.4f;
    player.gravity = (2.0f * max_jump) / powf(time_to_jump_apex, 2.0f);
    player.min_jump_speed = -powf(2.0f * fabs(player.gravity) * min_jump, 0.5f);
    player.max_jump_speed = -fabs(player.gravity) * time_to_jump_apex;
    player.fall_multiplier = 1.9f;
    player.coyote_time = 0.1f;
    player.coyote_time_left = 0.0f;
    player.jump_buffer = 0.15f;
    player.jump_buffer_left = 0.0f;
    player.is_grounded = false;

    float top_left_x;
    float bottom_right_x;
    float top_left_y;
    float bottom_right_y;

    while (!WindowShouldClose()) {
        int keycode_pressed = GetKeyPressed();

        bool jump_pressed = IsKeyPressed(player.keybinds[UP]);
        Vector2 input_dir = (Vector2){0.0f, 0.0f};
        if (IsKeyDown(player.keybinds[LEFT])) input_dir.x -= 1;
        if (IsKeyDown(player.keybinds[RIGHT])) input_dir.x += 1;
        if (IsKeyDown(player.keybinds[UP])) input_dir.y -= 1;
        if (IsKeyDown(player.keybinds[DOWN])) input_dir.y += 1;

        Vector2 move_dir = (Vector2){0.0f, 0.0f};
        if (player.vel.x > 0) move_dir.x = 1;
        else if (player.vel.x < 0) move_dir.x = -1;
        if (player.vel.y > 0) move_dir.y = 1;
        else if (player.vel.y < 0) move_dir.y = -1;

        // Set Buffers
        if (player.is_grounded) player.coyote_time_left = player.coyote_time;
        else if (player.coyote_time_left > 0) player.coyote_time_left -= FIXED_DT;

        if (jump_pressed) player.jump_buffer_left = player.jump_buffer;
        else if (player.jump_buffer_left > 0) player.jump_buffer_left -= FIXED_DT;

        // Player movement
        if (input_dir.x != move_dir.x) {
            player.vel.x = Lerp(0.0f, player.vel.x, powf(player.friction, FIXED_DT));
            if (fabs(player.vel.x) < 1) player.vel.x = 0;
        }

        if (
            player.jump_buffer_left > 0 && player.is_grounded ||
            jump_pressed && player.coyote_time_left > 0
        ) {
            player.vel.y = player.max_jump_speed;
            player.is_grounded = false;
            player.coyote_time_left = 0;
            player.jump_buffer_left = 0;
        }

        if (
            move_dir.y < 0 &&
            input_dir.y != -1 &&
            player.vel.y < player.min_jump_speed
        ) {
            player.vel.y = player.min_jump_speed;
        }

        float fall_speed = player.gravity;
        if (move_dir.y > 0) {
            fall_speed *= player.fall_multiplier;
        }

        Vector2 acc = (Vector2) {
            input_dir.x * player.speed * FIXED_DT,
            fall_speed * FIXED_DT
        };

        player.vel = Vector2Add(player.vel, acc);
        player.vel = Vector2Clamp(player.vel, player.min_vel, player.max_vel);

        // Check for collision, handle each axis separately
        player.aabb.x += player.vel.x * FIXED_DT;

        // Calculate cells in level that player is in and check for collision
        top_left_x = player.aabb.x / CELL_SIZE;
        bottom_right_x = (player.aabb.x + player.aabb.width) / CELL_SIZE;
        top_left_y = player.aabb.y / CELL_SIZE;
        bottom_right_y = (player.aabb.y + player.aabb.height) / CELL_SIZE;
        for (int y = top_left_y; y <= bottom_right_y; y++) {
            for (int x = top_left_x; x <= bottom_right_x; x++) {
                if (!inside_level(x, y)) continue;

                int cell_type = level[y][x];
                if (cell_type == EMPTY) continue;

                if (player.vel.x > 0) {
                    player.aabb.x = x * CELL_SIZE - player.aabb.width - SMOL;
                }
                else if (player.vel.x < 0) {
                    player.aabb.x = (x + 1) * CELL_SIZE + SMOL;
                }
                player.vel.x = 0;
            }
        }

        player.aabb.y += player.vel.y * FIXED_DT;
        player.is_grounded = false;

        // Calculate cells in level that player is in and check for collision
        top_left_x = player.aabb.x / CELL_SIZE;
        bottom_right_x = (player.aabb.x + player.aabb.width) / CELL_SIZE;
        top_left_y = player.aabb.y / CELL_SIZE;
        bottom_right_y = (player.aabb.y + player.aabb.height) / CELL_SIZE;
        for (int y = top_left_y; y <= bottom_right_y; y++) {
            for (int x = top_left_x; x <= bottom_right_x; x++) {
                if (!inside_level(x, y)) continue;

                int cell_type = level[y][x];
                if (cell_type == EMPTY) continue;

                if (player.vel.y > 0) {
                    player.aabb.y = y * CELL_SIZE - player.aabb.height - SMOL;
                    player.is_grounded = true;
                }
                else if (player.vel.y < 0) {
                    player.aabb.y = (y + 1) * CELL_SIZE + SMOL;
                }
                player.vel.y = 0;
            }
        }


        /*elapsed_time += FIXED_DT;*/
        /*SetShaderValue(shader, scanline_offset, &elapsed_time, SHADER_UNIFORM_FLOAT);*/

        // Render to a texture for textures affected by postprocessing shaders
        BeginTextureMode(target);
            BeginMode2D(camera);
                ClearBackground(BLACK);

                // Draw level
                for (int y = 0; y < LEVEL_HEIGHT; y++) {
                    for (int x = 0; x < LEVEL_WIDTH; x++) {
                        int cell_type = level[y][x];
                        if (cell_type == EMPTY) continue;
                        Rectangle cell_rect = {
                            x * CELL_SIZE,
                            y * CELL_SIZE,
                            CELL_SIZE,
                            CELL_SIZE
                        };
                        DrawRectangleLinesEx(cell_rect, 2.0f, BLUE);
                    }
                }

                // Draw player
                DrawRectangleRec(player.aabb, MAGENTA);
            EndMode2D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawTextureRec(target.texture, (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
            DrawFPS(0, 0);
        EndDrawing();
    }

    // De-Initialization
    UnloadShader(shader);
    UnloadRenderTexture(target);

    CloseWindow();
}


bool inside_level(int x, int y) {
    return (x >= 0 && x < LEVEL_WIDTH && y >= 0 && y < LEVEL_HEIGHT);
}
