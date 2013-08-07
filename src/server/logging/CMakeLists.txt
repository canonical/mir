set(
  LOGGING_SOURCES

  glog_logger.cpp
  session_mediator_report.cpp
  message_processor_report.cpp
  display_report.cpp
  input_report.cpp
)

add_library(
  mirlogging STATIC

  ${LOGGING_SOURCES}
)

target_link_libraries(
  mirlogging

  mirplatform

  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
)
