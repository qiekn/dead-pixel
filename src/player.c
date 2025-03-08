#include "raylib.h"
#include "raymath.h"
#include "settings.h"
#include "player.h"
#include "level.h"


void player_update(Player *player, int *level) {
    player->is_eating = false;
    if (!player->is_maxxed_out) {
        player->time_remaining--;
    }

    bool jump_pressed = IsKeyPressed(player->keybinds[A]);
    bool jump_held = IsKeyDown(player->keybinds[A]);
    bool shift_pressed = IsKeyPressed(player->keybinds[B]);
    bool shift_held = IsKeyDown(player->keybinds[B]);
    Vector2 input_dir = (Vector2){0.0f, 0.0f};
    if (IsKeyDown(player->keybinds[LEFT])) input_dir.x -= 1;
    if (IsKeyDown(player->keybinds[RIGHT])) input_dir.x += 1;
    if (IsKeyDown(player->keybinds[UP])) input_dir.y -= 1;
    if (IsKeyDown(player->keybinds[DOWN])) input_dir.y += 1;

    Vector2 move_dir = (Vector2){0.0f, 0.0f};
    if (player->vel.x > 0) move_dir.x = 1;
    else if (player->vel.x < 0) move_dir.x = -1;
    if (player->vel.y > 0) move_dir.y = 1;
    else if (player->vel.y < 0) move_dir.y = -1;

    // Reset size ( Must be before setting buffers )
    if (shift_pressed) {
        player->last_grow_direction = Vector2Zero();
        player->grow_x_colliding = false;
        player->grow_y_colliding = false;
        if (player->shift_buffer_left > 0 && Vector2Equals(input_dir, Vector2Zero()) && player->is_grounded) {
            player->aabb.x += player->aabb.width / 2.0f - MIN_PLAYER_SIZE / 2.0f;
            player->aabb.y += player->aabb.height / 2.0f - MIN_PLAYER_SIZE / 2.0f;
            player->aabb.width = MIN_PLAYER_SIZE;
            player->aabb.height = MIN_PLAYER_SIZE;
        }
    }

    // Set Buffers
    if (player->is_grounded) player->coyote_time_left = player->coyote_time;
    else if (player->coyote_time_left > 0) player->coyote_time_left -= FIXED_DT;

    if (jump_pressed) player->jump_buffer_left = player->jump_buffer;
    else if (player->jump_buffer_left > 0) player->jump_buffer_left -= FIXED_DT;

    if (shift_pressed) player->shift_buffer_left = player->shift_buffer;
    else if (player->shift_buffer_left > 0) player->shift_buffer_left -= FIXED_DT;

    player->is_shifting = shift_held;

    if (shift_held) {
        player->vel = Vector2Zero();
        if (input_dir.x != 0) {
            player->grow_y_colliding = false;
            if (input_dir.x != player->last_grow_direction.x) {
                player->grow_x_colliding = false;
            }
            int grow = player->grow_x_colliding ? -1 : 1;
            player->last_grow_direction.x = input_dir.x;

            float grow_speed = player->aabb.width / MAX_PLAYER_WIDTH * player->grow_speed_max + player->grow_speed_min;
            float old_x = player->aabb.x;
            float old_width = player->aabb.width;
            float new_width = old_width + grow * grow_speed * FIXED_DT;
            new_width = Clamp(new_width, MIN_PLAYER_SIZE, player->max_width + 1);
            float grow_difference = new_width - old_width;

            player->aabb.width = new_width;

            if ((input_dir.x < 0 && grow > 0) || (input_dir.x > 0 && grow < 0)) {
                player->aabb.x -= grow_difference;
            }

            Vector2 hit = rect_collision(player->aabb, level);
            if (!Vector2Equals(hit, Vector2Zero())) {
                player->grow_x_colliding = true;
                player->aabb.width = old_width;
                if (input_dir.x < 0) {
                    player->aabb.x += grow_difference;
                }
            }

            if (player->aabb.width > player->max_width) {
                player->aabb.width = old_width;
                player->aabb.x = old_x;
            }
        }
        if (input_dir.y != 0) {
            player->grow_x_colliding = false;
            if (input_dir.y != player->last_grow_direction.y) {
                player->grow_y_colliding = false;
            }
            int grow = player->grow_y_colliding ? -1 : 1;
            player->last_grow_direction.y = input_dir.y;

            float grow_speed = player->aabb.height / MAX_PLAYER_HEIGHT * player->grow_speed_max + player->grow_speed_min;
            float old_y = player->aabb.y;
            float old_height = player->aabb.height;
            float new_height = old_height + grow * grow_speed * FIXED_DT;
            new_height = Clamp(new_height, MIN_PLAYER_SIZE, player->max_height + 1);
            float grow_difference = new_height - old_height;

            player->aabb.height = new_height;

            if ((input_dir.y < 0 && grow > 0) || (input_dir.y > 0 && grow < 0)) {
                player->aabb.y -= grow_difference;
            }

            Vector2 hit = rect_collision(player->aabb, level);
            if (!Vector2Equals(hit, Vector2Zero())) {
                player->grow_y_colliding = true;
                player->aabb.height = old_height;
                if (input_dir.y < 0) {
                    player->aabb.y += grow_difference;
                }
            }

            if (player->aabb.height > player->max_height) {
                player->aabb.height = old_height;
                player->aabb.y = old_y;
            }
        }
    } else {
        // Player movement
        if (input_dir.x != move_dir.x) {
            player->vel.x = Lerp(0.0f, player->vel.x, powf(player->friction, FIXED_DT));
            if (fabs(player->vel.x) < 1) player->vel.x = 0;
        }

        if (
            (player->jump_buffer_left > 0 && player->is_grounded) ||
            (jump_pressed && player->coyote_time_left > 0)
        ) {
            player->vel.y = player->max_jump_speed;
            player->is_grounded = false;
            player->coyote_time_left = 0;
            player->jump_buffer_left = 0;
        }

        if (
            move_dir.y < 0 &&
            !jump_held &&
            player->vel.y < player->min_jump_speed
        ) {
            player->vel.y = player->min_jump_speed;
        }

        float fall_speed = player->gravity;
        if (move_dir.y > 0) {
            fall_speed *= player->fall_multiplier;
        }

        Vector2 acc = (Vector2) {
            input_dir.x * player->speed * FIXED_DT,
            fall_speed * FIXED_DT
        };

        player->vel = Vector2Add(player->vel, acc);
        player->vel = Vector2Clamp(player->vel, player->min_vel, player->max_vel);

        // Check for collision, handle each axis separately
        player->aabb.x += player->vel.x * FIXED_DT;

        // Calculate cells in level that player is in and check for collision
        Vector2 hit = rect_collision(player->aabb, level);
        if (!Vector2Equals(hit, Vector2Zero())) {
            if (player->vel.x > 0) {
                player->aabb.x = hit.x * CELL_SIZE - player->aabb.width - 1 - SMOL;
            }
            else {
                player->aabb.x = (hit.x + 1) * CELL_SIZE + 1 + SMOL;
            }
            player->vel.x = 0;
        }

        player->aabb.y += player->vel.y * FIXED_DT;
        player->is_grounded = false;

        // Calculate cells in level that player is in and check for collision
        hit = rect_collision(player->aabb, level);
        if (!Vector2Equals(hit, Vector2Zero())) {
            if (player->vel.y > 0) {
                player->aabb.y = hit.y * CELL_SIZE - player->aabb.height - SMOL;
                player->is_grounded = true;
            }
            else if (player->vel.y < 0) {
                player->aabb.y = (hit.y + 1) * CELL_SIZE + SMOL;
            }
            player->vel.y = 0;
        }
    }

    // Check for collision with collectables
    float top_left_x = player->aabb.x / CELL_SIZE;
    float bottom_right_x = (player->aabb.x + player->aabb.width) / CELL_SIZE;
    float top_left_y = player->aabb.y / CELL_SIZE;
    float bottom_right_y = (player->aabb.y + player->aabb.height) / CELL_SIZE;
    for (int y = top_left_y; y <= bottom_right_y; y++) {
        for (int x = top_left_x; x <= bottom_right_x; x++) {
            if (!inside_map(x, y)) continue;
            int cell_type = level[y * MAP_WIDTH + x];
            if (cell_type == VIRUS) {
                level[y * MAP_WIDTH + x] = EMPTY;
                player->is_eating = true;
                player->bugs_collected += VIRUS_VALUE;
            }
        }
    }
}

Vector2 rect_collision(Rectangle aabb, int *level) {
    float top_left_x = aabb.x / CELL_SIZE;
    float bottom_right_x = (aabb.x + aabb.width) / CELL_SIZE;
    float top_left_y = aabb.y / CELL_SIZE;
    float bottom_right_y = (aabb.y + aabb.height) / CELL_SIZE;
    for (int y = top_left_y; y <= bottom_right_y; y++) {
        for (int x = top_left_x; x <= bottom_right_x; x++) {
            if (!inside_map(x, y)) return (Vector2) {x, y};
            int cell_type = level[y * MAP_WIDTH + x];
            if (cell_type == SOLID) return (Vector2) {x, y};
        }
    }
    return Vector2Zero();
}
