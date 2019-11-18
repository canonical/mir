set(
  DECORATION_SOURCES

  manager.h
  null_manager.h
  decoration.h
)

add_library(
  mirshelldecoration OBJECT

  ${DECORATION_SOURCES}
)
