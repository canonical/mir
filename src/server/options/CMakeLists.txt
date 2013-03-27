set(
  CHOICE_SOURCES

  program_option.cpp
)

add_library(
  miroptions STATIC

  ${CHOICE_SOURCES}
)
