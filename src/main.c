#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdio.h>


#define PLATFORM_DESKTOP
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FPS 60
#define FIXED_DT (1.0f / FPS)

#define CELL_SIZE 16
#define LEVEL_WIDTH (WINDOW_WIDTH / CELL_SIZE)
#define LEVEL_HEIGHT (WINDOW_HEIGHT / CELL_SIZE)

#define MAX_KEYBINDS 6

const Vector2 WINDOW_CENTRE = (Vector2) {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
const float SMOL = 0.01f;
const float MIN_PLAYER_SIZE = CELL_SIZE - 4.0f;


// TODO: Add restart R, change to wasd jk r
typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    UP,
    A,
    B,
} Action;

typedef enum {
    EMPTY,
    SOLID,
    WATER, // TODO: Remove
} CellType;

// TODO: Store max width and max height separately
// Store collected bugs
typedef struct {
    int keybinds[MAX_KEYBINDS];
    Rectangle aabb;
    Vector2 vel;
    Vector2 max_vel;
    Vector2 min_vel;
    Vector2 last_grow_direction;
    float max_size;
    float grow_speed;
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
    float shift_buffer;
    float shift_buffer_left;
    bool is_grounded;
    bool grow_x_colliding;
    bool grow_y_colliding;
} Player;


bool inside_level(int x, int y);
Vector2 rect_collision(Rectangle aabb, int level[LEVEL_HEIGHT][LEVEL_WIDTH]);


int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    InitAudioDevice();
    HideCursor();
    SetTargetFPS(FPS);

    Music music = LoadMusicStream("src/resources/Tron Legacy - Son of Flynn (Remix).mp3");
    PlayMusicStream(music);

    Shader shader_scanlines = LoadShader(0, TextFormat("src/resources/shaders%i/scanlines.fs", GLSL_VERSION));
    Shader shader_blur = LoadShader(0, TextFormat("src/resources/shaders%i/blur.fs", GLSL_VERSION));

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    RenderTexture2D target_world = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

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

    Camera2D camera = {};
    camera.target = Vector2Zero();
    camera.offset = Vector2Zero();
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Player player = {0};
    player.keybinds[RIGHT] = 262;
    player.keybinds[LEFT] = 263;
    player.keybinds[DOWN] = 264;
    player.keybinds[UP] = 265;
    player.keybinds[A] = 90;
    player.keybinds[B] = 88;
    player.aabb = (Rectangle){
        WINDOW_CENTRE.x, WINDOW_CENTRE.y,
        CELL_SIZE * 4.0f - 1.0f, CELL_SIZE * 4.0f - 1.0f
    };
    player.vel = Vector2Zero();
    player.max_vel = (Vector2){200.0f, 400.0f};
    player.min_vel = Vector2Negate(player.max_vel);
    player.last_grow_direction = Vector2Zero();
    player.max_size = CELL_SIZE * 8.0f - 1.0f;
    player.grow_speed = 200.0f;
    player.speed = 800.0f;
    player.friction = 0.00001f;  // Between 0 - 1, higher means lower friction
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
    player.shift_buffer = 0.3f;
    player.shift_buffer_left = 0.0f;
    player.is_grounded = false;
    player.grow_x_colliding = false;
    player.grow_y_colliding = false;

    Vector2 hit = {0};

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        bool jump_pressed = IsKeyPressed(player.keybinds[A]);
        bool jump_held = IsKeyDown(player.keybinds[A]);
        bool shift_pressed = IsKeyPressed(player.keybinds[B]);
        bool shift_held = IsKeyDown(player.keybinds[B]);
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

        // Reset size ( Must be before setting buffers )
        if (shift_pressed) {
            player.last_grow_direction = Vector2Zero();
            player.grow_x_colliding = false;
            player.grow_y_colliding = false;
            if (player.shift_buffer_left > 0 && Vector2Equals(input_dir, Vector2Zero()) && player.is_grounded) {
                player.aabb.x += player.aabb.width / 2.0f - MIN_PLAYER_SIZE / 2.0f;
                player.aabb.y += player.aabb.height / 2.0f - MIN_PLAYER_SIZE / 2.0f;
                player.aabb.width = MIN_PLAYER_SIZE;
                player.aabb.height = MIN_PLAYER_SIZE;
            }
        }

        // Set Buffers
        if (player.is_grounded) player.coyote_time_left = player.coyote_time;
        else if (player.coyote_time_left > 0) player.coyote_time_left -= FIXED_DT;

        if (jump_pressed) player.jump_buffer_left = player.jump_buffer;
        else if (player.jump_buffer_left > 0) player.jump_buffer_left -= FIXED_DT;

        if (shift_pressed) player.shift_buffer_left = player.shift_buffer;
        else if (player.shift_buffer_left > 0) player.shift_buffer_left -= FIXED_DT;

        if (shift_held) {
            player.vel = Vector2Zero();
            if (input_dir.x != 0) {
                player.grow_y_colliding = false;
                if (input_dir.x != player.last_grow_direction.x) {
                    player.grow_x_colliding = false;
                }
                int grow = player.grow_x_colliding ? -1 : 1;
                player.last_grow_direction.x = input_dir.x;

                float old_x = player.aabb.x;
                float old_width = player.aabb.width;
                float new_width = old_width + grow * player.grow_speed * FIXED_DT;
                new_width = Clamp(new_width, MIN_PLAYER_SIZE, player.max_size + CELL_SIZE);
                float grow_difference = new_width - old_width;

                player.aabb.width = new_width;

                if ((input_dir.x < 0 && grow > 0) || (input_dir.x > 0 && grow < 0)) {
                    player.aabb.x -= grow_difference;
                }

                hit = rect_collision(player.aabb, level);
                if (!Vector2Equals(hit, Vector2Zero())) {
                    player.grow_x_colliding = true;
                    player.aabb.width = old_width;
                    if (input_dir.x < 0) {
                        player.aabb.x += grow_difference;
                    }
                }

                if (player.aabb.width > player.max_size) {
                    player.aabb.width = old_width;
                    player.aabb.x = old_x;
                }
            }
            if (input_dir.y != 0) {
                player.grow_x_colliding = false;
                if (input_dir.y != player.last_grow_direction.y) {
                    player.grow_y_colliding = false;
                }
                int grow = player.grow_y_colliding ? -1 : 1;
                player.last_grow_direction.y = input_dir.y;

                float old_y = player.aabb.y;
                float old_height = player.aabb.height;
                float new_height = old_height + grow * player.grow_speed * FIXED_DT;
                new_height = Clamp(new_height, MIN_PLAYER_SIZE, player.max_size + CELL_SIZE);
                float grow_difference = new_height - old_height;

                player.aabb.height = new_height;

                if ((input_dir.y < 0 && grow > 0) || (input_dir.y > 0 && grow < 0)) {
                    player.aabb.y -= grow_difference;
                }

                hit = rect_collision(player.aabb, level);
                if (!Vector2Equals(hit, Vector2Zero())) {
                    player.grow_y_colliding = true;
                    player.aabb.height = old_height;
                    if (input_dir.y < 0) {
                        player.aabb.y += grow_difference;
                    }
                }

                if (player.aabb.height > player.max_size) {
                    player.aabb.height = old_height;
                    player.aabb.y = old_y;
                }
            }
        } else {
            // Player movement
            if (input_dir.x != move_dir.x) {
                player.vel.x = Lerp(0.0f, player.vel.x, powf(player.friction, FIXED_DT));
                if (fabs(player.vel.x) < 1) player.vel.x = 0;
            }

            if (
                (player.jump_buffer_left > 0 && player.is_grounded) ||
                (jump_pressed && player.coyote_time_left > 0)
            ) {
                player.vel.y = player.max_jump_speed;
                player.is_grounded = false;
                player.coyote_time_left = 0;
                player.jump_buffer_left = 0;
            }

            if (
                move_dir.y < 0 &&
                !jump_held &&
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
            hit = rect_collision(player.aabb, level);
            if (!Vector2Equals(hit, Vector2Zero())) {
                if (player.vel.x > 0) {
                    player.aabb.x = hit.x * CELL_SIZE - player.aabb.width - SMOL;
                }
                else if (player.vel.x < 0) {
                    player.aabb.x = (hit.x + 1) * CELL_SIZE + SMOL;
                }
                player.vel.x = 0;
            }

            player.aabb.y += player.vel.y * FIXED_DT;
            player.is_grounded = false;

            // Calculate cells in level that player is in and check for collision
            hit = rect_collision(player.aabb, level);
            if (!Vector2Equals(hit, Vector2Zero())) {
                if (player.vel.y > 0) {
                    player.aabb.y = hit.y * CELL_SIZE - player.aabb.height - SMOL;
                    player.is_grounded = true;
                }
                else if (player.vel.y < 0) {
                    player.aabb.y = (hit.y + 1) * CELL_SIZE + SMOL;
                }
                player.vel.y = 0;
            }
        }

        // Render to a texture for textures affected by postprocessing shaders
        BeginTextureMode(target);
            BeginMode2D(camera);
                ClearBackground(BLACK);
                // Draw player
                DrawRectangleRec(player.aabb, MAGENTA);
            EndMode2D();
        EndTextureMode();

        BeginTextureMode(target_world);
            ClearBackground(BLACK);
            BeginMode2D(camera);
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
                        if (cell_type == SOLID) {
                            DrawRectangleLinesEx(cell_rect, 2.0f, BLUE);
                        } else if (cell_type == WATER) {
                            DrawRectangleRec(cell_rect, GREEN);
                        }
                    }
                }
            EndMode2D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader_blur);
                DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
            BeginShaderMode(shader_scanlines);
                DrawTextureRec(target.texture, (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
            /*DrawFPS(0, 0);*/
        EndDrawing();
    }

    // De-Initialization
    UnloadShader(shader_scanlines);
    UnloadRenderTexture(target);
    UnloadMusicStream(music);
    CloseAudioDevice();

    CloseWindow();
}


bool inside_level(int x, int y) {
    return (x >= 0 && x < LEVEL_WIDTH && y >= 0 && y < LEVEL_HEIGHT);
}

Vector2 rect_collision(Rectangle aabb, int level[LEVEL_HEIGHT][LEVEL_WIDTH]) {
    float top_left_x = aabb.x / CELL_SIZE;
    float bottom_right_x = (aabb.x + aabb.width) / CELL_SIZE;
    float top_left_y = aabb.y / CELL_SIZE;
    float bottom_right_y = (aabb.y + aabb.height) / CELL_SIZE;
    for (int y = top_left_y; y <= bottom_right_y; y++) {
        for (int x = top_left_x; x <= bottom_right_x; x++) {
            if (!inside_level(x, y)) continue;
            int cell_type = level[y][x];
            if (cell_type == EMPTY) continue;
            return (Vector2) {x, y};
        }
    }
    return Vector2Zero();
}
