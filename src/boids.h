#ifndef BOIDS_H
#define BOIDS_H


#include "raylib.h"
#include "player.h"
#include "settings.h"


#define NUM_BOIDS 4096
#define BOID_SIZE 6
#define VIEW_DISTANCE 64
#define VIEW_DISTANCE_SQR (VIEW_DISTANCE * VIEW_DISTANCE)
#define AVOID_DISTANCE 16
#define AVOID_DISTANCE_SQR (AVOID_DISTANCE * AVOID_DISTANCE)
#define VIEW_DOT_PRODUCT -0.6
#define SEPARATION_CONSTANT 1
#define ALIGNMENT_CONSTANT 0.05
#define COHESION_CONSTANT 0.01
#define AVOIDANCE_CONSTANT 4
#define MOVE_SPEED 120

#define PLAYER_AVOID_FACTOR 4

#define GRID_HALF_SIZE VIEW_DISTANCE
#define GRID_SIZE (GRID_HALF_SIZE * 2)
#define GRID_WIDTH (WORLD_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (WORLD_HEIGHT / GRID_SIZE)
#define GRID_CELLS (GRID_WIDTH * GRID_HEIGHT)


typedef struct {
    Vector2 position;
    Vector2 direction;
} Boid;


void setup_list(int link_heads[GRID_CELLS]);
void setup_linked_list(Boid boids[NUM_BOIDS], int link_heads[GRID_CELLS], int links[NUM_BOIDS]);
void update_boids(
    Player *player,
    Boid boids[NUM_BOIDS],
    int link_heads[GRID_CELLS],
    int links[NUM_BOIDS],
    Vector2 average_positions[NUM_BOIDS],
    Vector2 average_directions[NUM_BOIDS],
    Vector2 average_separations[NUM_BOIDS]
);

#endif  /* BOIDS_H */
