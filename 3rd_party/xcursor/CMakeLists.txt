set(
  XCURSOR_SOURCES

  xcursor.c
)

add_library(
  xcursorloader STATIC

  ${XCURSOR_SOURCES}
)
