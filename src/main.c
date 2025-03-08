#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "boids.h"
#include "level.h"
#include "settings.h"


#define CRASH_BLUE (Color) {16, 10, 209, 255}
#define CYAN (Color) {0, 255, 255, 255}
#define WALL_COLOUR (Color) {90, 150, 255, 255}
#define BOID_COLOUR (Color) {10, 255, 0, 150}
#define MUSIC_VOLUME 0.6f
#define MUSIC_VOLUME_QUIET 0.3f

#define RESTART_MESSAGE_LENGTH 350
#define UPGRADES 3
#define UPGRADE_LEVELS 8

#define WINDOW_CENTRE (Vector2) {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f}


typedef enum {
    STATE_GAME,
    STATE_UPGRADE,
    STATE_RELOAD,
    STATE_GAMEOVER,
} GameStates;


void restart_sequence(Player *player, char restart_message_target[RESTART_MESSAGE_LENGTH], char restart_message_live[RESTART_MESSAGE_LENGTH], int *char_index, int *delay_left);
void reset_game(Player *player, Boid *boids, int *link_heads);


int main(void) {
    GameStates current_state = STATE_RELOAD;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "playmakers-jam");
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(8192);
    HideCursor();
    SetTargetFPS(FPS);

    // Set to zero for easy bug reproducability.
    SetRandomSeed(0);

    Music music = LoadMusicStream("src/resources/Tron Legacy - Son of Flynn (Remix).ogg");
    SetMusicVolume(music, MUSIC_VOLUME);
    PlayMusicStream(music);

    Sound virus_sfx = LoadSound("src/resources/virus.ogg");
    Sound static_sfx = LoadSound("src/resources/static.ogg");

    Font font_c64 = LoadFont("src/resources/C64_Pro-STYLE.ttf");
    Font font_opensans = LoadFontEx("src/resources/OpenSans-Light.ttf", 256, NULL, 255);

    Mesh cubeMesh = GenMeshCube(1, 1, 1);
    Model cubeModel = LoadModelFromMesh(cubeMesh);
    /*cubeModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = texture;*/
    Camera camera3D = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };
    camera3D.fovy = 10.0f;
    camera3D.projection = CAMERA_ORTHOGRAPHIC;

    Shader shader_scanlines = LoadShader(0, TextFormat("src/resources/shaders%i/scanlines.fs", GLSL_VERSION));
    Shader shader_blur = LoadShader(0, TextFormat("src/resources/shaders%i/blur.fs", GLSL_VERSION));
    Shader shader_glitch = LoadShader(0, TextFormat("src/resources/shaders%i/glitch.fs", GLSL_VERSION));
    int timeLoc = GetShaderLocation(shader_glitch, "time");

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
    player.keybinds[START] = 32;
    player.aabb = (Rectangle){
        100, 500,
        MIN_PLAYER_SIZE, MIN_PLAYER_SIZE
    };
    player.vel = Vector2Zero();
    player.max_vel = (Vector2){200.0f, 400.0f};
    player.min_vel = Vector2Negate(player.max_vel);
    player.last_grow_direction = Vector2Zero();
    player.max_width = MIN_PLAYER_SIZE;
    player.max_height = MIN_PLAYER_SIZE;
    /*player.max_width = CELL_SIZE * 4.0f - 4.0f;*/
    /*player.max_height = CELL_SIZE * 4.0f - 4.0f;*/
    /*player.max_width = MAX_PLAYER_WIDTH;*/
    /*player.max_height = MAX_PLAYER_HEIGHT;*/
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
    /*player.bugs_collected = 999999;*/
    player.max_time = FPS * 25;  // FPS * Seconds
    player.time_remaining = player.max_time;
    player.is_grounded = false;
    player.is_shifting = false;
    player.is_eating = false;
    player.is_maxxed_out = false;
    player.grow_x_colliding = false;
    player.grow_y_colliding = false;

    /*char keycode[50] = "KEYCODE: 0";*/
    /*char timer[50] = "TIME REMAINING: 0";*/
    char bugs_collected_text[50];
    char upgrades_text[100];
    char upgrades_price_text[50];

    char restart_message_target[RESTART_MESSAGE_LENGTH];
    char restart_message_live[RESTART_MESSAGE_LENGTH];
    int char_index = 0;
    int delay_left = 0;
    restart_sequence(&player, restart_message_target, restart_message_live, &char_index, &delay_left);

    int upgrade_index = 0;
    int upgrade_level[UPGRADES] = {0};
    int upgrade_price[UPGRADE_LEVELS] = {8, 16, 32, 128, 256, 1024, 4096, 16384};

    float time;

    while (!WindowShouldClose()) {
        time = (float) GetTime();
        SetShaderValue(shader_glitch, timeLoc, &time, SHADER_UNIFORM_FLOAT);
        UpdateMusicStream(music);

        switch (current_state) {
        case STATE_GAME:
            // Update
            if (player.time_remaining > 0 || player.is_maxxed_out) {
                player_update(&player, level);
                update_boids(&player, boids, link_heads, average_positions, average_directions, average_separations);
            }

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

            /*int keycode_pressed = GetKeyPressed();*/
            /*if (keycode_pressed) sprintf(keycode, "KEYCODE: %d", keycode_pressed);*/

            /*sprintf(collected, "BUGS COLLECTED: %d", player.bugs_collected);*/
            /*sprintf(timer, "TIME REMAINING: %d", player.time_remaining);*/

            // RESTART
            if ((IsKeyPressed(player.keybinds[RESTART]) && !player.is_maxxed_out) || (player.time_remaining == 0 && !player.is_maxxed_out)) {
                restart_sequence(&player, restart_message_target, restart_message_live, &char_index, &delay_left);
                current_state = STATE_RELOAD;
            }

            // WIN
            if (player.is_maxxed_out && player.aabb.width >= MAX_PLAYER_WIDTH - 10 && player.aabb.height >= MAX_PLAYER_HEIGHT - 10 && !player.is_shifting) {
                current_state = STATE_GAMEOVER;
                continue;
            }

            if (player.is_eating && !IsSoundPlaying(virus_sfx)) {
                PlaySound(virus_sfx);
            }

            // Render to a texture for textures affected by postprocessing shaders
            BeginTextureMode(target_entities);
                ClearBackground(BLACK);
                BeginMode2D(camera);
                    // Draw player
                    Color player_colour = player.is_maxxed_out ? CRASH_BLUE : MAGENTA;
                    if (player.is_eating) {
                        player_colour = YELLOW;
                    } else if (player.is_shifting) {
                        player_colour = CYAN;
                    }
                    DrawRectangleRec(player.aabb, player_colour);
                EndMode2D();
            EndTextureMode();

            BeginTextureMode(target_world);
                ClearBackground(BLACK);
                BeginMode2D(camera);
                    // Draw boids
                    for (int i = 0; i < NUM_BOIDS; i++) {
                        if (boids[i].eaten) continue;
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
                            BOID_COLOUR
                        );
                    }
                EndMode2D();

                // Draw level
                for (int y = 0; y < LEVEL_HEIGHT; y++) {
                    for (int x = 0; x < LEVEL_WIDTH; x++) {
                        int cell_type = level[(int)((y + render_offset.y) * MAP_WIDTH + x + render_offset.x)];
                        if (cell_type == EMPTY) continue;
                        if (cell_type == VIRUS) {
                            DrawCircleLines(x * CELL_SIZE, y * CELL_SIZE, 6, YELLOW);
                        } else {
                            Rectangle cell_rect = {
                                x * CELL_SIZE,
                                y * CELL_SIZE,
                                CELL_SIZE,
                                CELL_SIZE
                            };
                            /*DrawRectangleLinesEx(cell_rect, 2.0f, WALL_COLOUR);*/
                            DrawRectangleRec(cell_rect, BLACK);
                            DrawRectangleRoundedLinesEx(cell_rect, 0.2f, 0, 2.0f, WALL_COLOUR);
                        }
                    }
                }
            EndTextureMode();

            BeginDrawing();
                ClearBackground(BLACK);
                if ((player.time_remaining < TIME_REMAINING_GLITCH && !player.is_maxxed_out) || (player.time_remaining > player.max_time - FPS / 2 && !player.is_maxxed_out)) {
                    if (!IsSoundPlaying(static_sfx)) {
                        PlaySound(static_sfx);
                    }
                    BeginShaderMode(shader_glitch);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                } else {
                    BeginShaderMode(shader_blur);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                }
                BeginShaderMode(shader_scanlines);
                    DrawTextureRec(target_entities.texture, (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height}, (Vector2){0, 0}, WHITE);
                EndShaderMode();
                /*DrawFPS(10, 0);*/
                /*DrawText(keycode, 10, 20, 20, WHITE);*/
                /*DrawText(collected, 10, 40, 20, YELLOW);*/
                /*DrawText(timer, 10, 60, 20, MAGENTA);*/
                DrawRectangleLinesEx((Rectangle) {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT}, 2.0f, BLACK);
            EndDrawing();
            break;

        case STATE_UPGRADE:
            SetMusicVolume(music, MUSIC_VOLUME);

            if (!player.is_maxxed_out) {
                sprintf(upgrades_text, "WIDTH: %d\t\t[%d/%d]\n\n\nHEIGHT: %d\t\t[%d/%d]\n\n\nTIME: %d\t\t[%d/%d]", (int)player.max_width, upgrade_level[0], UPGRADE_LEVELS, (int)player.max_height, upgrade_level[1], UPGRADE_LEVELS, player.max_time, upgrade_level[2], UPGRADE_LEVELS);
            } else {
                sprintf(upgrades_text, "WIDTH: 999999\t\tMAX\n\n\nHEIGHT: 999999\t\tMAX\n\n\nTIME: 999999\t\tMAX");
            }

            if (IsKeyPressed(player.keybinds[UP])) {
                upgrade_index--;
                if (upgrade_index < 0) upgrade_index = UPGRADES - 1;
            }
            if (IsKeyPressed(player.keybinds[DOWN])) {
                upgrade_index++;
                if (upgrade_index >= UPGRADES) upgrade_index = 0;
            }
            if (IsKeyPressed(player.keybinds[A])) {
                int level = upgrade_level[upgrade_index];
                if (level < UPGRADE_LEVELS) {
                    int price = upgrade_price[level];
                    if (player.bugs_collected >= price) {
                        float factor = (level+1);
                        factor *= factor / 2;
                        if (upgrade_index == 0) {
                            player.max_width += 7 * factor + 10;
                        } else if (upgrade_index == 1) {
                            player.max_height += 4 * factor + 10;
                        } else {
                            player.max_time *= 1.5;
                        }
                        upgrade_level[upgrade_index]++;
                        player.bugs_collected -= price;

                        PlaySound(virus_sfx);

                        // Check for maxxed
                        if (upgrade_level[0] == UPGRADE_LEVELS && upgrade_level[1] == UPGRADE_LEVELS && upgrade_level[2] == UPGRADE_LEVELS) {
                            player.max_width = MAX_PLAYER_WIDTH;
                            player.max_height = MAX_PLAYER_HEIGHT;
                            player.max_time = MAX_PLAYER_TIME;
                            player.is_maxxed_out = true;
                        }
                    }
                }
            }
            else if (IsKeyPressed(player.keybinds[START])) {
                reset_game(&player, boids, link_heads);
                current_state = STATE_GAME;
            }

            sprintf(bugs_collected_text, "BUGS COLLECTED: %d", player.bugs_collected);

            int level = upgrade_level[upgrade_index];
            if (level < UPGRADE_LEVELS) {
                int price = upgrade_price[level];
                sprintf(upgrades_price_text, "%d BUGS", price);
            } else {
                sprintf(upgrades_price_text, "CANNOT UPGRADE");
            }

            BeginTextureMode(target_entities);
                ClearBackground(BLACK);
                BeginMode3D(camera3D);
                    DrawModelEx(
                        cubeModel,
                        (Vector3){0, 0, 0},
                        (Vector3){0.2f, 0.8f, 0.1f},
                        time * 100.0f,
                        (Vector3){player.max_width / MAX_PLAYER_WIDTH * 10, player.max_height/ MAX_PLAYER_HEIGHT * 10, player.max_height / MAX_PLAYER_HEIGHT * 10},
                        player.is_maxxed_out ? CRASH_BLUE : MAGENTA
                    );
                    /*DrawModelWiresEx(cubeModel, (Vector3){0, 0, 0}, (Vector3){0, 1.0f, 0}, time*10, (Vector3){1.0f, 1.0f, 1.0f}, WHITE);*/
                    /*DrawCubeV(Vector3Zero(), (Vector3){1, 1, 1}, WHITE);*/
                    /*DrawCubeWiresV(Vector3Zero(), (Vector3){1, 1, 1}, MAGENTA);*/
                EndMode3D();
            EndTextureMode();

            Rectangle selection_rect = (Rectangle) {10, upgrade_index * 77 + 290, 440, 40};

            BeginTextureMode(target_world);
                ClearBackground(BLACK);
                DrawTextEx(
                    font_c64,
                    "DEAD PIXEL",
                    (Vector2){10, 10}, 80, 1, WHITE
                );
                DrawTextEx(
                    font_c64,
                    "CONTROLS:\n\n\n<WASD> To move and stretch\n\n<J> To jump and select\n\n<K> (HOLD) To stretch\n\n    (TAP 2x) To shrink\n\n\n\n<R> To manually restart\n\n<SPACE> To start\n\n\n\n\n\n\n           SEBZANARDO 2025",
                    (Vector2){830, 300}, 16, 1,
                    GRAY
                );
                DrawTextEx(
                    font_c64,
                    upgrades_text,
                    (Vector2){30, 300}, 24, 1,
                    YELLOW
                );
                DrawTextEx(
                    font_c64,
                    bugs_collected_text,
                    (Vector2){30, 550}, 16, 1,
                    GREEN
                );
                DrawRectangleRoundedLinesEx(selection_rect, 0.1f, 4.0f, 4.0f, MAGENTA);
                DrawRectangle(25, upgrade_index * 77 + 280, 220, 20, BLACK);
                DrawTextEx(
                    font_c64,
                    upgrades_price_text,
                    (Vector2){30, upgrade_index * 77 + 280}, 16, 1,
                    GREEN
                );
            EndTextureMode();

            BeginDrawing();
                if (player.is_maxxed_out) {
                    if (!IsSoundPlaying(static_sfx)) {
                        PlaySound(static_sfx);
                    }
                    BeginShaderMode(shader_glitch);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                } else {
                    BeginShaderMode(shader_blur);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                }

                BeginShaderMode(shader_scanlines);
                    DrawTextureRec(target_entities.texture, (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height}, (Vector2){0, 0}, WHITE);
                EndShaderMode();
            EndDrawing();
            break;

        case STATE_RELOAD:
            SetMusicVolume(music, MUSIC_VOLUME_QUIET);
            if (char_index < RESTART_MESSAGE_LENGTH) {
                if (delay_left > 0) {
                    delay_left--;
                } else {
                    for (int i = 0; i < 20; i++) {
                        char c = restart_message_target[char_index];
                        restart_message_live[char_index] = c;
                        restart_message_live[++char_index] = '\0';
                        if (c == '*' || c == '\n') {
                            break;
                        }
                        if (c == '-') {
                            delay_left += 2;
                            break;
                        }
                    }
                    delay_left += 1;
                }
            } else {
                current_state = STATE_UPGRADE;
            }

            BeginTextureMode(target_entities);
                ClearBackground(BLACK);
                EndMode3D();
            EndTextureMode();

            BeginTextureMode(target_world);
                ClearBackground(BLACK);
                DrawTextEx(
                    font_c64,
                    restart_message_live,
                    (Vector2){10, 10}, 16, 1, WHITE
                );
            EndTextureMode();

            BeginDrawing();
                if (char_index > 300) {
                    if (!IsSoundPlaying(static_sfx)) {
                        PlaySound(static_sfx);
                    }
                    BeginShaderMode(shader_glitch);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                } else {
                    BeginShaderMode(shader_blur);
                        DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                    EndShaderMode();
                }

                BeginShaderMode(shader_scanlines);
                    DrawTextureRec(target_entities.texture, (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height}, (Vector2){0, 0}, WHITE);
                EndShaderMode();
            EndDrawing();
            break;

        case STATE_GAMEOVER:
            SetMusicVolume(music, MUSIC_VOLUME_QUIET);
            BeginTextureMode(target_world);
                ClearBackground(CRASH_BLUE);
                DrawTextEx(font_c64, "DEAD PIXEL", (Vector2) {162, 440}, 16, 0, WALL_COLOUR);
            EndTextureMode();

            BeginTextureMode(target_entities);
                ClearBackground(BLACK);
                DrawTextEx(font_opensans, ":)", (Vector2){100, 70}, 256, 1, WHITE);
                DrawTextEx(
                    font_c64,
                    "Your PC ran into a problem...\n\nThe           has grown too strong and has now\ncorrupted your entire hard drive. We're just collecting some\nerror info, and then you'll need to reinstall your operating system.\n\n\n100% Complete\nCongratulations!",
                    (Vector2){100, 405}, 16, 0, WHITE
                );
            EndTextureMode();

            BeginDrawing();
                BeginShaderMode(shader_glitch);
                    DrawTextureRec(target_world.texture, (Rectangle){0, 0, (float)target_world.texture.width, (float)-target_world.texture.height}, (Vector2){0, 0}, WHITE);
                EndShaderMode();
                BeginShaderMode(shader_scanlines);
                    DrawTextureRec(target_entities.texture, (Rectangle){0, 0, (float)target_entities.texture.width, (float)-target_entities.texture.height}, (Vector2){0, 0}, WHITE);
                EndShaderMode();
            EndDrawing();
            break;
        }
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

void restart_sequence(Player *player, char restart_message_target[RESTART_MESSAGE_LENGTH], char restart_message_live[RESTART_MESSAGE_LENGTH], int *char_index, int *delay_left) {
    sprintf(restart_message_target, "SEGMENTATION FAULT (Core Dumped)\nRestarting, please wait.../DEAD-PIXEL\nFreeing unused kernel memory\n\n-\n\nINIT: version 1.0.0 booting\n\n[ OK ] Found save file\n     * MAXIMUM_WIDTH = %d\n     * MAXIMUM_HEIGHT = %d\n     * TIME = %d\n     * BUGS COLLECTED = %d\n\n-\n\nRestart Successful!\n\n---\n\n\n\n\n\n", player->max_width == MAX_PLAYER_WIDTH ? 999999 : (int)player->max_width, player->max_height == MAX_PLAYER_HEIGHT ?  999999 : (int)player->max_height, player->is_maxxed_out ? 999999: player->max_time, player->bugs_collected);
    sprintf(restart_message_live, "");
    *char_index = 0;
    *delay_left = 0;
}

void reset_game(Player *player, Boid *boids, int *link_heads) {
    player->aabb = (Rectangle){
        144, 500,
        MIN_PLAYER_SIZE, MIN_PLAYER_SIZE
    };
    player->vel = Vector2Zero();
    player->last_grow_direction = Vector2Zero();
    player->coyote_time_left = 0.0f;
    player->jump_buffer_left = 0.0f;
    player->shift_buffer_left = 0.0f;
    player->time_remaining = player->max_time;
    player->is_grounded = false;
    player->is_shifting = false;
    player->is_eating = false;
    player->grow_x_colliding = false;
    player->grow_y_colliding = false;

    setup_list(link_heads);
    setup_linked_list(boids, link_heads);
}
