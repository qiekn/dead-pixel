#ifndef LEVEL_H
#define LEVEL_H


#include "stdbool.h"
#include "settings.h"


typedef enum {
    EMPTY,
    SOLID,
} CellType;


bool load_level(int level[LEVEL_HEIGHT][LEVEL_WIDTH]);
bool inside_level(int x, int y);



#endif  /* LEVEL_H */
