#ifndef LEVEL_H
#define LEVEL_H


#include "stdbool.h"
#include "settings.h"


typedef enum {
    EMPTY,
    SOLID,
} CellType;


bool load_level(int level[MAP_HEIGHT][MAP_WIDTH]);
bool inside_map(int x, int y);


#endif  /* LEVEL_H */
