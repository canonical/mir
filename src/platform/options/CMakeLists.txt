set(
  CHOICE_SOURCES

  program_option.cpp
  default_configuration.cpp
)

add_library(miroptions OBJECT

  ${CHOICE_SOURCES}
)

target_link_libraries(miroptions
  PUBLIC
    mircommon
    mircore
)
