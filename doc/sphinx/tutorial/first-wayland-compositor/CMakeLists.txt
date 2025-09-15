cmake_minimum_required(VERSION 3.16)

project(demo-mir-compositor)

set(CMAKE_CXX_STANDARD 23)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED IMPORTED_TARGET)

add_executable(demo-mir-compositor main.cpp)

target_link_libraries(demo-mir-compositor PkgConfig::MIRAL)
