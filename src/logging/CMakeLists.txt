set(
  LOGGING_SOURCES

  dumb_console_logger.cpp
  application_mediator_report.cpp
  display_report.cpp
)

add_library(
  mirlogging STATIC

  ${LOGGING_SOURCES}
)
