cmake_minimum_required(VERSION 3.14)

project(gameoflife LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)
set(SFML_USE_STATIC_STD_LIBS ON)

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -funroll-loops -march=native -mtune=native")
endif()

# SFML dependencies:
# sudo apt install libxrandr-dev libxcursor-dev libudev-dev libfreetype-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
# sudo dnf install freetype-devel libvorbis-devel flac-devel systemd-devel # CSFML-devel

include(FetchContent)

FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.x
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
  poolSTL
  GIT_REPOSITORY https://github.com/alugowski/poolSTL.git
  GIT_TAG main
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(poolSTL)

add_executable(gameoflife main.cpp Grid.hpp DoubleBuffer.hpp)
target_link_libraries(gameoflife PRIVATE SFML::Graphics poolSTL::poolSTL)
