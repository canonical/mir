include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
set(
  LOGGING_SOURCES

  default_configuration.cpp
  glog_logger.cpp
)

add_library(
  mirlogger OBJECT

  ${LOGGING_SOURCES}
)

