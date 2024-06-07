set(
  DECORATION_SOURCES

  ${PROJECT_SOURCE_DIR}/src/include/server/mir/shell/decoration/manager.h
  null_manager.h
  basic_manager.h       basic_manager.cpp
  threadsafe_access.h
  decoration.h
  basic_decoration.h    basic_decoration.cpp
  window.h              window.cpp
  input.h               input.cpp
  renderer.h            renderer.cpp
)

add_library(
  mirshelldecoration OBJECT

  ${DECORATION_SOURCES}
)

target_link_libraries(mirshelldecoration
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::FREETYPE
)
