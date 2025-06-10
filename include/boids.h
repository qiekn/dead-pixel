#pragma once

#include "player.h"
#include "raylib.h"
#include "settings.h"

static const int NUM_BOIDS = 2048;
static const float BOID_SIZE = 6;
static const int VIEW_DISTANCE = 64;
static const int VIEW_DISTANCE_SQR = (VIEW_DISTANCE * VIEW_DISTANCE);
static const int AVOID_DISTANCE = 12;
static const int AVOID_DISTANCE_SQR = (AVOID_DISTANCE * AVOID_DISTANCE);
static const float VIEW_DOT_PRODUCT = -0.6;
static const float SEPARATION_CONSTANT = 0.3;
static const float ALIGNMENT_CONSTANT = 0.02;
static const float COHESION_CONSTANT = 0.05;
static const float AVOIDANCE_CONSTANT = 0.1;
static const int MOVE_SPEED = 110;

static const int PLAYER_AVOID_BUFFER = CELL_SIZE * 2.5;

static const int GRID_HALF_SIZE = VIEW_DISTANCE;
static const int GRID_SIZE = (GRID_HALF_SIZE * 2);
static const int GRID_WIDTH = (WORLD_WIDTH / GRID_SIZE);
static const int GRID_HEIGHT = (WORLD_HEIGHT / GRID_SIZE);
static const int GRID_CELLS = (GRID_WIDTH * GRID_HEIGHT);

typedef struct {
  Vector2 position;
  Vector2 direction;
  int next;
  bool eaten;
} Boid;

void setup_list(int *link_heads);
void setup_linked_list(Boid *boids, int *link_heads);
void update_boids(Player *player, Boid *boids, int *link_heads, Vector2 *average_positions, Vector2 *average_directions,
                  Vector2 *average_separations);
