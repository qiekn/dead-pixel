#ifndef SETTINGS_H
#define SETTINGS_H


/*#define PLATFORM_DESKTOP*/
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif

// TODO: Oh gosh these need to be const
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define FPS 60
#define FIXED_DT (1.0f / FPS)

#define CELL_SIZE 16
#define LEVEL_WIDTH (WINDOW_WIDTH / CELL_SIZE)
#define LEVEL_HEIGHT (WINDOW_HEIGHT / CELL_SIZE)
#define MAP_WIDTH (LEVEL_WIDTH * 4)
#define MAP_HEIGHT (LEVEL_HEIGHT * 4)
#define WORLD_WIDTH (MAP_WIDTH * CELL_SIZE)
#define WORLD_HEIGHT (MAP_HEIGHT * CELL_SIZE)


#endif  /* SETTINGS_H */
