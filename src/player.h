#ifndef PLAYER_H
#define PLAYER_H


#include "raylib.h"
#include "settings.h"


static const float SMOL = 0.01f;
static const int MIN_PLAYER_SIZE = (CELL_SIZE - 4.0f);
static const int MAX_PLAYER_WIDTH = (WINDOW_WIDTH - 6.0f);
static const int MAX_PLAYER_HEIGHT = (WINDOW_HEIGHT - 6.0f);
static const int MAX_PLAYER_TIME = -999;
static const int TIME_REMAINING_GLITCH = FPS * 10;


#define MAX_KEYBINDS 8


typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    UP,
    A,
    B,
    RESTART,
    START,
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
    float grow_speed_max;
    float grow_speed_min;
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
    int max_time;
    int time_remaining;
    bool is_grounded;
    bool is_shifting;
    bool is_eating;
    bool is_maxxed_out;
    bool grow_x_colliding;
    bool grow_y_colliding;
} Player;


void player_update(Player *player, int *level);
Vector2 rect_collision(Rectangle aabb, int *level);


#endif  /* PLAYER_H */
