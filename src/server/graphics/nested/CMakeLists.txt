include_directories(
    ${MIRCLIENT_INCLUDE_DIRS}
)

add_library(
  mirnestedgraphics STATIC

  nested_platform.cpp
)

target_link_libraries(
  mirnestedgraphics
)
