cmake_minimum_required(VERSION 3.14)

project(gameoflife VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

set(SFML_USE_STATIC_STD_LIBS ON)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL3IMAGE_AVIF OFF)
set(SDL3IMAGE_INSTALL OFF)
set(SDL3IMAGE_BUILD_SHARED_LIBS OFF)

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -funroll-loops")
  set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
endif()

# SFML dependencies:
# sudo apt install libxrandr-dev libxcursor-dev libudev-dev libfreetype-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
# sudo dnf install freetype-devel libvorbis-devel flac-devel systemd-devel # CSFML-devel

include(FetchContent)

FetchContent_Declare(
  poolSTL
  GIT_REPOSITORY https://github.com/alugowski/poolSTL.git
  GIT_TAG main
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(poolSTL)

FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.x
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.x
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(SDL3)

# FetchContent_Declare(
#     SDL3_image
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
#     GIT_TAG release-3.2.x
#     GIT_SHALLOW ON
#     EXCLUDE_FROM_ALL
#     OVERRIDE_FIND_PACKAGE)
# FetchContent_MakeAvailable(SDL3_image)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(raylib)

add_library(gameoflife INTERFACE Grid.hpp DoubleBuffer.hpp Common.hpp)
target_link_libraries(gameoflife INTERFACE poolSTL::poolSTL)

add_executable(gameoflife-sfml main-sfml.cpp)
target_link_libraries(gameoflife-sfml PRIVATE gameoflife SFML::Graphics)

add_executable(gameoflife-sdl main-sdl.cpp)
target_link_libraries(gameoflife-sdl PRIVATE gameoflife SDL3::SDL3-static) # SDL3_image::SDL3_image-static
target_compile_definitions(gameoflife-sdl PRIVATE SDL_MAIN_USE_CALLBACKS=1) # use the callbacks instead of main()

add_executable(gameoflife-raylib main-raylib.cpp)
target_link_libraries(gameoflife-raylib PRIVATE gameoflife raylib)
