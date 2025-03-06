#ifndef PLAYER_H
#define PLAYER_H


#include "raylib.h"
#include "settings.h"


#define SMOL 0.01f
#define MIN_PLAYER_SIZE (CELL_SIZE - 4.0f)
#define MAX_KEYBINDS 7


typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    UP,
    A,
    B,
    RESTART,
} Action;

typedef struct {
    int keybinds[MAX_KEYBINDS];
    Rectangle aabb;
    Vector2 vel;
    Vector2 max_vel;
    Vector2 min_vel;
    Vector2 last_grow_direction;
    float max_width;
    float max_height;
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
    int bugs_collected;
    bool is_grounded;
    bool grow_x_colliding;
    bool grow_y_colliding;
} Player;


void player_update(Player *player, int level[LEVEL_HEIGHT][LEVEL_WIDTH]);
Vector2 rect_collision(Rectangle aabb, int level[LEVEL_HEIGHT][LEVEL_WIDTH]);


#endif  /* PLAYER_H */
