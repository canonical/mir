set(
  DECORATION_SOURCES

  basic_manager.h 	basic_manager.cpp
  basic_decoration.h 	basic_decoration.cpp
  input.h 		input.cpp
  renderer.h		renderer.cpp
  window.h		window.cpp
  null_manager.h
  threadsafe_access.h
  decoration_notifier.cpp
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
