project(Mir)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x -O2 -Wall")

enable_testing()

include_directories(include/)

add_subdirectory(tests/)
