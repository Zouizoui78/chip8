cmake_minimum_required(VERSION 3.0.0)

set(PROJECT emu-chip8)
project(${PROJECT} VERSION 0.0.1)

# C++ variables
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# OS defines used inside code
if (WIN32)
    add_compile_options(-DWINDOWS)
endif (WIN32)
if (UNIX)
    add_compile_options(-DLINUX)
endif (UNIX)

add_compile_options($<$<CONFIG:Debug>:-DDEBUG>$<$<CONFIG:Release>:-DRELEASE>)

set(
    SRC
    src/main.cpp
    src/Chip8.cpp
)

add_executable(${PROJECT} ${SRC})