cmake_minimum_required(VERSION 3.15)
project(DEAD_PIXEL C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

file(GLOB SRCS
    src/*.c
)

add_executable(game ${SRCS})

find_package(raylib REQUIRED)

if(APPLE)
    target_link_libraries(game raylib "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(game raylib GL m pthread dl rt X11)
elseif(WIN32)
    target_link_libraries(game raylib opengl32 gdi32 winmm)
endif()
