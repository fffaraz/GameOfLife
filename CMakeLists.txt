cmake_minimum_required(VERSION 3.5)

project(gameoflife LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML dependencies: sudo apt install libxrandr-dev libxcursor-dev libudev-dev
# libfreetype-dev libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev
# libegl1-mesa-dev

include(FetchContent)
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

add_executable(gameoflife main.cpp)
target_link_libraries(gameoflife PRIVATE sfml-graphics)
