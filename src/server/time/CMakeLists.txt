set(
  TIME_SOURCES

  alarm_factory.cpp
  ${CMAKE_SOURCE_DIR}/src/include/server/mir/time/alarm_factory.h
)

add_library(
  mirservertime OBJECT

  ${TIME_SOURCES}
)

target_link_libraries(mirservertime
  PUBLIC
    mircommon
)
