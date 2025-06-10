#pragma once

#define PLATFORM_DESKTOP

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else
#define GLSL_VERSION 100
#endif

static const int WINDOW_WIDTH = 1280;
static const int WINDOW_HEIGHT = 720;
static const int FPS = 60;
static const float FIXED_DT = (1.0f / FPS);
