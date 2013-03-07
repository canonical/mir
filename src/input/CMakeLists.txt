set(
  INPUT_SOURCES

  event_filter_chain.cpp
)

if (NOT MIR_DISABLE_INPUT)
add_subdirectory(android)
else()
list(
  APPEND INPUT_SOURCES
  dummy_input_manager.cpp
)
endif()

add_library(
  mirinput STATIC

  ${INPUT_SOURCES}
)
uses_android_input(mirinput)
