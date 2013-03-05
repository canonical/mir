include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  DRAW_SRCS

  graphics_utils.cpp
)

add_library(
  mirdraw STATIC
  ${DRAW_SRCS})

target_link_libraries(
  mirdraw

  ${GLESv2_LIBRARIES}
 )
