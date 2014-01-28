include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  DRAW_SRCS

  patterns.cpp
)

if (MIR_PLATFORM STREQUAL "android")
  set(
    DRAW_SRCS

    android_graphics_region_factory.cpp
    ${DRAW_SRCS}
    )
endif()

add_library(
  mirtestdraw STATIC
  ${DRAW_SRCS})

target_link_libraries(
  mirtestdraw

  ${LIBHARDWARE_LIBRARIES}
 )
