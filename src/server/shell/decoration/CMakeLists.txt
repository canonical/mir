set(
  DECORATION_SOURCES

  manager.h
  null_manager.h
  basic_manager.h basic_manager.cpp
  decoration.h
)

add_library(
  mirshelldecoration OBJECT

  ${DECORATION_SOURCES}
)
