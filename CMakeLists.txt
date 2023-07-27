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
    src/files.cpp
    src/Scheduler.cpp
    src/Stopwatch.cpp
    src/sdl/InputMapper.cpp
    src/sdl/Sound.cpp
    src/sdl/Window.cpp
)

add_executable(${PROJECT} ${SRC})

target_include_directories(
    ${PROJECT}
    PRIVATE src
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(emu-chip8 PRIVATE spdlog::spdlog)

target_link_libraries(
    emu-chip8
    PRIVATE
    nlohmann_json::nlohmann_json
)

target_link_libraries(
    emu-chip8
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

target_link_libraries(
    emu-chip8
    PRIVATE
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
)
