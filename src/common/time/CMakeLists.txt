ADD_LIBRARY(
  mirtime OBJECT

  steady_clock.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/time/posix_clock.h
  posix_clock.cpp
)
