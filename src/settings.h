#ifndef SETTINGS_H
#define SETTINGS_H


/*#define PLATFORM_DESKTOP*/
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif


static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;
static const int FPS = 60;
static const float FIXED_DT = (1.0f / FPS);

static const int CELL_SIZE = 16;
static const int LEVEL_WIDTH = (WINDOW_WIDTH / CELL_SIZE);
static const int LEVEL_HEIGHT = (WINDOW_HEIGHT / CELL_SIZE);
static const int MAP_WIDTH = (LEVEL_WIDTH * 4);
static const int MAP_HEIGHT = (LEVEL_HEIGHT * 4);
static const int WORLD_WIDTH = (MAP_WIDTH * CELL_SIZE);
static const int WORLD_HEIGHT = (MAP_HEIGHT * CELL_SIZE);


#endif  /* SETTINGS_H */
