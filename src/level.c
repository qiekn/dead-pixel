#include "level.h"
#include "settings.h"
#include "stdio.h"

bool load_level(int* level) {
  FILE* file_ptr = fopen("assets/map.txt", "r");

  if (file_ptr == NULL) {
    printf("ERROR: Could not open file!\n");
    return false;
  }

  // TODO: Make this better. Load file into array and resize array???
  int x = 0;
  int y = 0;
  int cell_type;
  while (fscanf(file_ptr, "%d", &cell_type) == 1 && y < MAP_HEIGHT) {
    level[y * MAP_WIDTH + x] = cell_type;
    x++;
    if (fgetc(file_ptr) == '\n' || x >= MAP_WIDTH) {
      x = 0;
      y++;
    }
  }

  fclose(file_ptr);
  return true;
}

bool inside_map(int x, int y) { return (x > 0 && x < MAP_WIDTH && y > 0 && y < MAP_HEIGHT); }
