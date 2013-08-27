set(
  INPUT_SOURCES

  event_filter_chain.cpp

  null_input_configuration.cpp
  null_input_report.cpp
  nested_input_configuration.cpp
  display_input_region.cpp
  vt_filter.cpp
)

add_subdirectory(android)

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
