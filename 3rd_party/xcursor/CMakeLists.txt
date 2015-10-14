set(
  XCURSOR_SOURCES

  xcursor.c
)

add_library(
  xcursorloader OBJECT

  ${XCURSOR_SOURCES}
)
