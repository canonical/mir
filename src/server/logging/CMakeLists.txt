include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
set(
  LOGGING_SOURCES

  default_configuration.cpp
  glog_logger.cpp
)

add_library(
  mirlogger STATIC

  ${LOGGING_SOURCES}
)

target_link_libraries(
  mirlogger

  mirplatform

  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
)
