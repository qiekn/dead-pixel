#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "boids.h"
#include "level.h"
#include "settings.h"


#define CYAN (Color) {0, 255, 255, 255}
#define WINDOW_CENTRE (Vector2) {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f}

void restart(Player *player, Boid *boids, int *link_heads);


int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(8192);
    HideCursor();
    SetTargetFPS(FPS);

    // Set to zero for easy bug reproducability.
    // TODO: Change to be time for final build
    SetRandomSeed(0);

    Music music = LoadMusicStream("src/resources/Tron Legacy - Son of Flynn (Remix).ogg");
    PlayMusicStream(music);

    Shader shader_scanlines = LoadShader(0, TextFormat("src/resources/shaders%i/scanlines.fs", GLSL_VERSION));
    Shader shader_blur = LoadShader(0, TextFormat("src/resources/shaders%i/blur.fs", GLSL_VERSION));

    RenderTexture2D target_entities = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    RenderTexture2D target_world = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    int *level = (int *)malloc(sizeof(int) * MAP_WIDTH * MAP_HEIGHT);
    if (!load_level(level)) return 1;

    Boid *boids = (Boid *)malloc(sizeof(Boid) * NUM_BOIDS);
    int *link_heads = (int *)malloc(sizeof(int) * GRID_CELLS);
    setup_list(link_heads);
    setup_linked_list(boids, link_heads);
    Vector2 *average_positions = (Vector2 *)malloc(sizeof(Vector2) * NUM_BOIDS);
    Vector2 *average_directions = (Vector2 *)malloc(sizeof(Vector2) * NUM_BOIDS);
    Vector2 *average_separations = (Vector2 *)malloc(sizeof(Vector2) * NUM_BOIDS);

    Camera2D camera = {};
    camera.target = Vector2Zero();
    camera.offset = Vector2Zero();
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Player player = {0};
    player.keybinds[RIGHT] = 68;
    player.keybinds[LEFT] = 65;
    player.keybinds[DOWN] = 83;
    player.keybinds[UP] = 87;
    player.keybinds[A] = 74;
    player.keybinds[B] = 75;
    player.keybinds[RESTART] = 82;
    player.aabb = (Rectangle){
        4000, 2000,
        MIN_PLAYER_SIZE, MIN_PLAYER_SIZE
    };
    player.vel = Vector2Zero();
    player.max_vel = (Vector2){200.0f, 400.0f};
    player.min_vel = Vector2Negate(player.max_vel);
    player.last_grow_direction = Vector2Zero();
    player.max_width = WINDOW_WIDTH - 4.0f;
    player.max_height = WINDOW_HEIGHT - 4.0f;
    player.grow_speed_max = 400.0f;
    player.grow_speed_min = 100.0f;
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
    player.bugs_collected = 0;
    player.is_grounded = false;
    player.is_shifting = false;
    player.grow_x_colliding = false;
    player.grow_y_colliding = false;

    char keycode[50] = "KEYCODE: 0";

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        // Update
        int keycode_pressed = GetKeyPressed();
        if (keycode_pressed) sprintf(keycode, "KEYCODE: %d", keycode_pressed);

        player_update(&player, level);

        Vector2 player_centre = (Vector2) {
            player.aabb.x + player.aabb.width / 2,
            player.aabb.y + player.aabb.height / 2
        };

        Vector2 level_offset = (Vector2) {
            (int)(player_centre.x / WINDOW_WIDTH),
            (int)(player_centre.y / WINDOW_HEIGHT)
        };

        Vector2 render_offset = (Vector2) {
            level_offset.x * LEVEL_WIDTH,
            level_offset.y * LEVEL_HEIGHT
        };

        Vector2 camera_offset = (Vector2) {
            level_offset.x * WINDOW_WIDTH,
            level_offset.y * WINDOW_HEIGHT
        };

        camera.target = camera_offset;

        update_boids(&player, boids, link_heads, average_positions, average_directions, average_separations);

        // RESTART
        if (IsKeyPressed(player.keybinds[RESTART])) restart(&player, boids, link_heads);

        // Render to a texture for textures affected by postprocessing shaders
        BeginTextureMode(target_entities);
            BeginMode2D(camera);
                ClearBackground(BLACK);
                // Draw player
                DrawRectangleRec(player.aabb, player.is_shifting ? CYAN : MAGENTA);

                // Draw boids
                for (int i = 0; i < NUM_BOIDS; i++) {
                    if (
                        boids[i].position.x < render_offset.x * CELL_SIZE ||
                        boids[i].position.x > render_offset.x * CELL_SIZE + WINDOW_WIDTH ||
                        boids[i].position.y < render_offset.y * CELL_SIZE ||
                        boids[i].position.y > render_offset.y * CELL_SIZE + WINDOW_HEIGHT
                    ) continue;
                    /*DrawCircleLinesV(boids[i].position, VIEW_DISTANCE, GREEN);*/
                    /*DrawCircleLinesV(boids[i].position, AVOID_DISTANCE, BLUE);*/
                    /*DrawLineV(boids[i].position, Vector2Add(boids[i].position, Vector2Scale(boids[i].direction, AVOID_DISTANCE)), BLUE);*/
                    /*DrawCircleV(boids[i].position, BOID_SIZE, MAGENTA);*/
                    DrawTriangleLines(
                        Vector2Add(boids[i].position, Vector2Scale(boids[i].direction, AVOID_DISTANCE)),
                        (Vector2) {boids[i].position.x - BOID_SIZE, boids[i].position.y + BOID_SIZE},
                        (Vector2) {boids[i].position.x + BOID_SIZE, boids[i].position.y + BOID_SIZE},
                        GREEN
                    );
                }
            EndMode2D();
        EndTextureMode();

        BeginTextureMode(target_world);
            ClearBackground(BLACK);
            // Draw level
            for (int y = 0; y < LEVEL_HEIGHT; y++) {
                for (int x = 0; x < LEVEL_WIDTH; x++) {
                    int cell_type = level[(int)((y + render_offset.y) * MAP_WIDTH + x + render_offset.x)];
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
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader_blur);
                DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
            BeginShaderMode(shader_scanlines);
                DrawTextureRec(target_entities.texture, (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height}, (Vector2){0, 0}, WHITE);
            EndShaderMode();
            DrawFPS(0, 0);
            DrawText(keycode, 0, 20, 20, WHITE);
        EndDrawing();
    }

    // De-Initialization
    free(level);
    free(boids);
    free(link_heads);
    free(average_positions);
    free(average_directions);
    free(average_separations);
    UnloadShader(shader_scanlines);
    UnloadRenderTexture(target_entities);
    UnloadMusicStream(music);
    CloseAudioDevice();

    CloseWindow();
}

void restart(Player *player, Boid *boids, int *link_heads) {
    player->aabb = (Rectangle){
        WINDOW_CENTRE.x, WINDOW_CENTRE.y,
        MIN_PLAYER_SIZE, MIN_PLAYER_SIZE
    };
    player->vel = Vector2Zero();
    player->last_grow_direction = Vector2Zero();
    player->coyote_time_left = 0.0f;
    player->jump_buffer_left = 0.0f;
    player->shift_buffer_left = 0.0f;
    player->is_grounded = false;
    player->is_shifting = false;
    player->grow_x_colliding = false;
    player->grow_y_colliding = false;

    setup_list(link_heads);
    setup_linked_list(boids, link_heads);
}
