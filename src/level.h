#ifndef LEVEL_H
#define LEVEL_H


#include "raylib.h"
#include "stdbool.h"
#include "settings.h"


#define VIRUS_VALUE 300


typedef enum {
    EMPTY,
    SOLID,
    VIRUS,
} CellType;


bool load_level(int *level);
bool inside_map(int x, int y);


#endif  /* LEVEL_H */
