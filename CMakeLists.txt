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

add_subdirectory("${CMAKE_SOURCE_DIR}/external/raylib")
target_link_libraries(Tetris PUBLIC raylib)


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

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)
