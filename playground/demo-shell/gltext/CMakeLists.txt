find_package(PkgConfig)

pkg_search_module(FREETYPE freetype2)
if (FREETYPE_FOUND)
  set(OPTIONAL_SRCS gltext_freetype_renderer.cpp)
endif ()

add_library(gltext STATIC
  gltext.cpp
  gltext_stub_renderer.cpp
  ${OPTIONAL_SRCS}
)

target_include_directories(gltext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (FREETYPE_FOUND)
  target_link_libraries(gltext ${FREETYPE_LIBRARIES})
  target_include_directories(gltext PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif ()

