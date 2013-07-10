include(ExternalProject)
include(FindPackageHandleStandardArgs)

#gtest
set(GTEST_INSTALL_DIR /usr/src/gmock/gtest/include)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h
            HINTS ${GTEST_INSTALL_DIR})

#gmock
set(GMOCK_INSTALL_DIR /usr/src/gmock)
find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)

set(GTEST_ALL_INCLUDES ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})

set(GMOCK_PREFIX gmock)
set(GMOCK_BINARY_DIR ${CMAKE_BINARY_DIR}/${GMOCK_PREFIX}/libs)
set(GTEST_BINARY_DIR ${GMOCK_BINARY_DIR}/gtest)
ExternalProject_Add(
    GMock
    #where to build in source tree
    PREFIX ${GMOCK_PREFIX}
    #where the source is external to the project
    SOURCE_DIR ${GMOCK_INSTALL_DIR}
    #forward the compilers to the subproject so cross-arch builds work
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    BINARY_DIR ${GMOCK_BINARY_DIR}

    #we don't need to install, so skip
    INSTALL_COMMAND ""
)

set(GMOCK_LIBRARY ${GMOCK_BINARY_DIR}/libgmock.a)
set(GMOCK_MAIN_LIBRARY ${GMOCK_BINARY_DIR}/libgmock_main.a)
set(GMOCK_BOTH_LIBRARIES ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY})
set(GTEST_LIBRARY ${GTEST_BINARY_DIR}/libgtest.a)
set(GTEST_MAIN_LIBRARY ${GTEST_BINARY_DIR}/libgtest_main.a)
set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
set(GTEST_ALL_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES})

# handle the QUIETLY and REQUIRED arguments and set EGL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GTest  DEFAULT_MSG
                                  GTEST_ALL_LIBRARIES GTEST_ALL_INCLUDES)

