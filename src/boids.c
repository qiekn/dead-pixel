#include <math.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"
#include "boids.h"
#include "settings.h"
#include "player.h"


void setup_list(int *link_heads) {
    // Setting up boids lists
    for (int i = 0; i < GRID_CELLS; i++) {
        link_heads[i] = -1;
    }
}


void setup_linked_list(Boid *boids, int *link_heads) {
    for (int i = 0; i < NUM_BOIDS; i++) {
        Vector2 position = (Vector2) {
            GetRandomValue(2, MAP_WIDTH - 2) * CELL_SIZE,
            GetRandomValue(2, MAP_HEIGHT - 2) * CELL_SIZE
        };
        Vector2 direction = Vector2Normalize((Vector2) {
            GetRandomValue(-64, 64),
            GetRandomValue(-64, 64)
        });
        boids[i] = (Boid) {position, direction, -1, false};

        int x_grid = (int) position.x / GRID_SIZE;
        x_grid = Wrap(x_grid, 0, GRID_WIDTH);

        int y_grid = (int) position.y / GRID_SIZE;
        y_grid = Wrap(y_grid, 0, GRID_HEIGHT);

        int i_grid = y_grid * GRID_WIDTH + x_grid;

        // There is no head of the list for this grid cell
        if (link_heads[i_grid] == -1) {
            link_heads[i_grid] = i;
        } else {
            boids[i].next = link_heads[i_grid];
            link_heads[i_grid] = i;
        }
    }
}


void update_boids(
    Player *player,
    Boid *boids,
    int *link_heads,
    Vector2 *average_positions,
    Vector2 *average_directions,
    Vector2 *average_separations
) {
    // Update Boids
    for (int i = 0; i < GRID_CELLS; i++) {
        if (link_heads[i] == -1) continue;

        int current = link_heads[i];
        while (current != -1) {
            // Calculate surrounding grid cells
            int x_grid = (int) boids[current].position.x / GRID_SIZE;
            x_grid = Wrap(x_grid, 0, GRID_WIDTH);
            int y_grid = (int) boids[current].position.y / GRID_SIZE;
            y_grid = Wrap(y_grid, 0, GRID_HEIGHT);

            int remaining_x = (int) boids[current].position.x - x_grid * GRID_SIZE;
            int remaining_y = (int) boids[current].position.y - y_grid * GRID_SIZE;

            int horizontal = 0;  // -1 or 1
            int vertical = 0;  // -1 or 1

            if (remaining_x >= GRID_HALF_SIZE) horizontal++;
            else if (remaining_x < GRID_HALF_SIZE) horizontal--;
            if (remaining_y >= GRID_HALF_SIZE) vertical++;
            else if (remaining_y < GRID_HALF_SIZE) vertical--;

            int count = 0;
            int separation_count = 0;
            Vector2 average_position = Vector2Zero();
            Vector2 average_direction = Vector2Zero();
            Vector2 average_separation = Vector2Zero();

            // Check 2x2 around boid for accurate movement
            int cell_to_check = i;
            for (int y = 0; y != vertical * 2; y += vertical) {
                cell_to_check = i + GRID_WIDTH * y;
                for (int x = 0; x != horizontal * 2; x += horizontal) {
                    cell_to_check += x;

                    if (cell_to_check < 0 || cell_to_check >= GRID_CELLS) continue;

                    int inside = link_heads[cell_to_check];
                    while ((inside != -1)) {
                        if (inside == current) {
                            inside = boids[inside].next;
                            continue;
                        }

                        float distance_sqr = Vector2DistanceSqr(boids[current].position, boids[inside].position);
                        if (distance_sqr > VIEW_DISTANCE_SQR) {
                            inside = boids[inside].next;
                            continue;
                        }
                        if (Vector2DotProduct(boids[current].position, boids[inside].position) < VIEW_DOT_PRODUCT) {
                            inside = boids[inside].next;
                            continue;
                        }
                        average_position = Vector2Add(average_position, boids[inside].position);
                        average_direction = Vector2Add(average_direction, boids[inside].direction);
                        count++;

                        if (distance_sqr > AVOID_DISTANCE_SQR) {
                            inside = boids[inside].next;
                            continue;
                        }
                        average_separation = Vector2Subtract(average_separation, Vector2Scale(Vector2Subtract(boids[current].position, boids[inside].position), 1.0f / distance_sqr));
                        separation_count++;

                        inside = boids[inside].next;
                    }
                }
            }

            average_positions[current] = Vector2Scale(average_position, 1.0f / count);
            average_directions[current] = Vector2Normalize(Vector2Scale(average_direction, 1.0f / count));
            average_separations[current] = Vector2Normalize(Vector2Scale(average_separation, 1.0f / separation_count));

            current = boids[current].next;
        }
    }

    Vector2 player_centre_pos = (Vector2) {player->aabb.x + player->aabb.width / 2, player->aabb.y + player->aabb.height / 2};
    Vector2 collection_distance = (Vector2) {player->aabb.width / 2, player->aabb.height / 2};
    Vector2 escape_distance = Vector2AddValue(collection_distance, PLAYER_AVOID_BUFFER);

    // Update boids linked list
    for (int cell = 0; cell < GRID_CELLS; cell++) {
        if (link_heads[cell] == -1) continue;

        int last = -1;
        int i = link_heads[cell];
        while (i != -1) {
            // Move boids
            // Separation
            Vector2 separation = Vector2Scale(Vector2Normalize(Vector2Subtract(average_separations[i], boids[i].direction)), -SEPARATION_CONSTANT);

            // Alignment
            Vector2 alignment = Vector2Scale(Vector2Normalize(Vector2Subtract(average_directions[i], boids[i].direction)), ALIGNMENT_CONSTANT);

            // Cohension
            Vector2 cohesion = Vector2Scale(Vector2Normalize(Vector2Subtract(average_positions[i], boids[i].position)), COHESION_CONSTANT);

            // Avoidance
            Vector2 avoidance = Vector2Zero();
            Vector2 distance = Vector2Subtract(boids[i].position, player_centre_pos);
            if (fabs(distance.x) < escape_distance.x && fabs(distance.y) < escape_distance.y) {
                avoidance = Vector2Scale(distance, AVOIDANCE_CONSTANT);
                if (fabs(distance.x) < collection_distance.x && fabs(distance.y) < collection_distance.y) {
                    boids[i].eaten = true;
                    player->is_eating = true;
                    player->bugs_collected++;
                    // TODO: Instantiate particle effect here
                }
            }

            // Update position & direction
            if (!player->is_shifting) {
                boids[i].direction = Vector2Normalize(Vector2Add(boids[i].direction, separation));
                boids[i].direction = Vector2Normalize(Vector2Add(boids[i].direction, alignment));
                boids[i].direction = Vector2Normalize(Vector2Add(boids[i].direction, cohesion));
                boids[i].direction = Vector2Normalize(Vector2Add(boids[i].direction, avoidance));
                boids[i].position = Vector2Add(boids[i].position, Vector2Scale(boids[i].direction, MOVE_SPEED * FIXED_DT));

                boids[i].position.x = Wrap(boids[i].position.x, 0, WORLD_WIDTH);
                boids[i].position.y = Wrap(boids[i].position.y, 0, WORLD_HEIGHT);
            }

            // Does this boid need to move cells?
            int x_grid = (int) boids[i].position.x / GRID_SIZE;
            x_grid = Wrap(x_grid, 0, GRID_WIDTH);
            int y_grid = (int) boids[i].position.y / GRID_SIZE;
            y_grid = Wrap(y_grid, 0, GRID_HEIGHT);
            int i_grid = y_grid * GRID_WIDTH + x_grid;

            // Boid still in valid grid cell
            if (cell == i_grid && !boids[i].eaten) {
                last = i;
                i = boids[i].next;
                continue;
            }

            // We gotta move the boid from this linked list to another
            // Move to head of other linked list for simplicity

            int old_next = boids[i].next;

            // Move head
            if (i == link_heads[cell]) {
                // Head now points to next element
                link_heads[cell] = boids[i].next;
            } else {
                // Last node point to current's next node
                boids[last].next = boids[i].next;
            }

            if (!boids[i].eaten) {
                // Assign removed node to be new head of correct cell
                boids[i].next = link_heads[i_grid];
                link_heads[i_grid] = i;
            }
            i = old_next;
        }
    }
}
