# CMake.
cmake_minimum_required(VERSION 3.15)

# Project.
project(Tetris VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# C Standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Executable
add_executable(Tetris tetris.c)

find_package(SDL2 REQUIRED)
target_link_libraries(Tetris PUBLIC SDL2::SDL2)
target_link_libraries(Tetris PUBLIC SDL2_image)
target_link_libraries(Tetris PUBLIC SDL2_ttf)
target_link_libraries(Tetris PUBLIC SDL2_mixer)


target_compile_options(Tetris PRIVATE
  -Wpedantic
  -Wall
  -Wextra
  -Wimplicit-fallthrough
  $<$<CONFIG:DEBUG>:-g3>
  $<$<CONFIG:DEBUG>:-Og>
  $<$<CONFIG:RELEASE>:-O3>
  $<$<CONFIG:RELEASE>:-Werror>
  -g
)
