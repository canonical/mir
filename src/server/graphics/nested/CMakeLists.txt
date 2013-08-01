include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirnestedgraphics STATIC

  nested_platform.cpp
)

target_link_libraries(
  mirnestedgraphics
)
