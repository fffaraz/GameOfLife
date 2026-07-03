cmake_minimum_required(VERSION 3.14)

project(gameoflife LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

# SFML dependencies:
# sudo apt install libxrandr-dev libxcursor-dev libudev-dev libfreetype-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
# sudo dnf install freetype-devel libvorbis-devel flac-devel systemd-devel # CSFML-devel

set(SFML_USE_STATIC_STD_LIBS ON)
set(SFML_USE_SYSTEM_DEPS OFF)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_X11_XSCRNSAVER OFF)
set(SDL_X11_XTEST OFF)

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -funroll-loops -flto")
  if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
  endif()
endif()

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
  GIT_TAG 3.1.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.12
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(raylib)

add_library(gameoflife Grid.hpp BandExecutor.hpp DoubleBuffer.hpp Common.hpp Common.cpp)
target_link_libraries(gameoflife PUBLIC poolSTL::poolSTL)

add_executable(gameoflife-sfml main-sfml.cpp)
target_link_libraries(gameoflife-sfml PRIVATE gameoflife SFML::Graphics)

add_executable(gameoflife-sdl main-sdl.cpp)
target_link_libraries(gameoflife-sdl PRIVATE gameoflife SDL3::SDL3-static)
target_compile_definitions(gameoflife-sdl PRIVATE SDL_MAIN_USE_CALLBACKS=1) # use the callbacks instead of main()

add_executable(gameoflife-raylib main-raylib.cpp)
target_link_libraries(gameoflife-raylib PRIVATE gameoflife raylib)

add_executable(gameoflife-cli main-cli.cpp)
target_link_libraries(gameoflife-cli PRIVATE gameoflife)
