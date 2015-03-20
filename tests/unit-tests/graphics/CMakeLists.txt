
add_subdirectory(egl_mock/)
if (MIR_TEST_PLATFORM STREQUAL "android")
add_subdirectory(android/)
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa")
#add_subdirectory(mesa/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
