set(
  DECORATION_SOURCES

  manager.h
  null_manager.h
  basic_manager.h       basic_manager.cpp
  threadsafe_access.h
  decoration.h
  basic_decoration.h    basic_decoration.cpp
  window.h              window.cpp
  input.h               input.cpp
  decoration_strategy.h decoration_strategy.cpp
)

add_library(
  mirshelldecoration OBJECT

  ${DECORATION_SOURCES}
)

target_include_directories(mirshelldecoration
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirshelldecoration
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::FREETYPE
)
