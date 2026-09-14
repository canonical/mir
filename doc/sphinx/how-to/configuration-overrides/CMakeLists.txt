cmake_minimum_required(VERSION 3.25)

project(demo-config-override)

set(CMAKE_CXX_STANDARD 26)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED IMPORTED_TARGET)

add_executable(demo-config-override main.cpp)

target_link_libraries(demo-config-override PkgConfig::MIRAL)
