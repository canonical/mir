set(
  INPUT_SOURCES

  event_filter_chain.cpp
)

if (NOT MIR_DISABLE_INPUT)
add_subdirectory(android)
else()
list(
  APPEND INPUT_SOURCES
  null_input_platform.cpp
)
endif()

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
