
# Pulling in local gmock version
# While this seems evil, we are doing
# it to ensure/allow for:
#   (1.) a matching gmock/gtest version
#   (2.) a stream-lined cross-compilation workflow
#set(
#  GTEST_INCLUDE_DIR

#  3rd_party/gmock/include
#  3rd_party/gmock/gtest/include)

#add_subdirectory(3rd_party/gmock/)
#set(GMOCK_LIBRARY gmock)
#set(GMOCK_MAIN_LIBRARY gmock_main)
#set(
#  GTEST_BOTH_LIBRARIES

#  ${GMOCK_LIBRARY}
#  ${GMOCK_MAIN_LIBRARY})

#set(GTEST_FOUND TRUE)
# GMock/GTest build finished and available to subsequent components
#include_directories (
#  ${GTEST_INCLUDE_DIR}
#)
include(ExternalProject)
include(FindPackageHandleStandardArgs)

#gtest
set(GTEST_INSTALL_DIR /usr/src/gmock/gtest/include)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h
            HINTS ${GTEST_INSTALL_DIR})
message("TAKING " ${GTEST_INCLUDE_DIR})



#gmock
set(GMOCK_INSTALL_DIR /usr/src/gmock)
find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)

set(GMOCK_PREFIX gmock)
set(GMOCK_BINARY_DIR ${CMAKE_BINARY_DIR}/${GMOCK_PREFIX}/libs)
set(GTEST_BINARY_DIR ${GMOCK_BINARY_DIR}/gtest)

ExternalProject_Add(
    GMock
    #where to build in source tree
    PREFIX ${GMOCK_PREFIX}
    #where the source is external to the project
    SOURCE_DIR ${GMOCK_INSTALL_DIR}
    BINARY_DIR ${GMOCK_BINARY_DIR}

    #we don't need to install, so skip
    INSTALL_COMMAND ""
)


set(GTEST_ALL_INCLUDES ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})

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

