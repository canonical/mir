set(
  INPUT_SOURCES

  event_filter_chain.cpp

  surface_data_storage.cpp
  null_input_configuration.cpp
  null_input_report.cpp
)

add_subdirectory(android)

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
