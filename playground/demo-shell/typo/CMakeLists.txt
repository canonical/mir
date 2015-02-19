find_package(GLESv2 REQUIRED)
find_package(PkgConfig)

pkg_search_module(FREETYPE freetype2)
if (FREETYPE_FOUND)
  set(OPTIONAL_SRCS typo_freetype_renderer.cpp)
endif ()

add_library(typo STATIC
  typo_renderer.cpp
  typo_stub_renderer.cpp
  typo_glcache.cpp
  ${OPTIONAL_SRCS}
)

target_link_libraries(typo ${GLESv2_LIBRARIES})
include_directories(${GLESv2_INCLUDE_DIRS})
target_include_directories(typo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (FREETYPE_FOUND)
  target_compile_definitions(typo PUBLIC -DTYPO_SUPPORTS_FREETYPE)
  target_link_libraries(typo ${FREETYPE_LIBRARIES})
  target_include_directories(typo PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif ()

