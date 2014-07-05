include_directories(
    ${PROJECT_SOURCE_DIR}/include/server
)

add_library(
  mirnestedscene STATIC

  default_host_lifecycle_event_listener.cpp
)

target_link_libraries(
  mirnestedscene
)
