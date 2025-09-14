cmake_minimum_required(VERSION 3.14)

project(gameoflife LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

if (NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# SFML dependencies:
# sudo apt install libxrandr-dev libxcursor-dev libudev-dev libfreetype-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
# sudo dnf install freetype-devel libvorbis-devel flac-devel systemd-devel # CSFML-devel

include(FetchContent)
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.x)
FetchContent_MakeAvailable(SFML)

add_executable(gameoflife main.cpp Grid.hpp DoubleBuffer.hpp Utils.hpp)
target_link_libraries(gameoflife PRIVATE sfml-graphics)
