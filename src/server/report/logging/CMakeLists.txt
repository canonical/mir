include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
set(
  LOGGING_SOURCES

  connector_report.cpp
  session_mediator_report.cpp
  message_processor_report.cpp
  display_report.cpp
  input_report.cpp
  compositor_report.cpp
  scene_report.cpp
  logging_report_factory.cpp
  display_configuration_report.cpp)

add_library(
  mirlogging OBJECT

  ${LOGGING_SOURCES}
)

