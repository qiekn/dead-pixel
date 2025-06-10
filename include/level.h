#pragma once

#include "raylib.h"
#include "settings.h"
#include "stdbool.h"

#define VIRUS_VALUE 300

typedef enum {
  EMPTY,
  SOLID,
  VIRUS,
} CellType;

bool load_level(int *level);
bool inside_map(int x, int y);
