set(
  INPUT_SOURCES

  event_filter_chain.cpp
  rectangles_input_region.cpp

  null_input_configuration.cpp
  null_input_report.cpp
)

add_subdirectory(android)

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
