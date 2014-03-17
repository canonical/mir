include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include/client
)

set(
  DRAW_SRCS

  patterns.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "android")
  include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
  add_definitions(-DANDROID)
  set(DRAW_SRCS
    android_graphics_region_factory.cpp
    ${DRAW_SRCS})
else()
   set(DRAW_SRCS
    mesa_graphics_region_factory.cpp
    ${DRAW_SRCS})
endif()

add_library(
  mirtestdraw STATIC
  ${DRAW_SRCS})

target_link_libraries(
  mirtestdraw

  ${LIBHARDWARE_LIBRARIES}
 )
