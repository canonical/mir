include(FindPackageHandleStandardArgs)

find_package(GTest)

if (NOT GTEST_FOUND)
    include(ExternalProject)

    find_path(GTEST_ROOT
        NAMES CMakeLists.txt
        PATHS /usr/src/gtest /usr/src/googletest/googletest/
        DOC "Path to GTest CMake project")

    ExternalProject_Add(GTest PREFIX ./gtest
            SOURCE_DIR ${GTEST_ROOT}
            CMAKE_ARGS
                -DCMAKE_CXX_COMPILER_WORKS=1
                -DCMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            INSTALL_COMMAND true)

    ExternalProject_Get_Property(GTest binary_dir)

    add_library(gtest UNKNOWN IMPORTED)
    set_target_properties(gtest      PROPERTIES IMPORTED_LOCATION ${binary_dir}/libgtest.a)
    add_dependencies(gtest GTest)
    set(GTEST_LIBRARY "gtest")

    add_library(gtest_main UNKNOWN IMPORTED)
    set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION ${binary_dir}/libgtest_main.a)
    add_dependencies(gtest_main GTest)
    set(GTEST_MAIN_LIBRARY "gtest_main")

    set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
    find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h)
    find_package_handle_standard_args(GTest GTEST_LIBRARY GTEST_BOTH_LIBRARIES GTEST_INCLUDE_DIRS)
endif()

find_file(GMOCK_SOURCE
        NAMES gmock-all.cc
        DOC "GMock source"
        PATHS /usr/src/googletest/googlemock/src/ /usr/src/gmock/ /usr/src/gmock/src)

message(STATUS "GMOCK_SOURCE=${GMOCK_SOURCE}")

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)

add_library(GMock STATIC ${GMOCK_SOURCE})

if (EXISTS /usr/src/googletest/googlemock/src)
    set_source_files_properties(${GMOCK_SOURCE} PROPERTIES COMPILE_FLAGS "-I/usr/src/googletest/googlemock")
endif()

if (EXISTS /usr/src/gmock/src)
    set_source_files_properties(${GMOCK_SOURCE} PROPERTIES COMPILE_FLAGS "-I/usr/src/gmock")
endif()

find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_INCLUDE_DIR)

set(GMOCK_LIBRARY GMock)
set(GMOCK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARY})
