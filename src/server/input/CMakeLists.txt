set(
  INPUT_SOURCES

  event_filter_chain.cpp
  null_input_configuration.cpp
)

add_subdirectory(android)

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
