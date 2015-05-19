set(
  XCURSOR_SOURCES

  xcursor.c
)

add_library(
  xcursorloader-static STATIC

  ${XCURSOR_SOURCES}
)
