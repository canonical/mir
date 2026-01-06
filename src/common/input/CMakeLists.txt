add_library(mircommoninput OBJECT
  mir_input_config.cpp        ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_input_config.h
  mir_keyboard_config.cpp     ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_keyboard_config.h
  mir_pointer_config.cpp      ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_pointer_config.h
  mir_touchpad_config.cpp     ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchpad_config.h
  mir_touchscreen_config.cpp  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchscreen_config.h
  parameter_keymap.cpp
  xkb_mapper.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/keyboard_leds.h
)

target_include_directories(mircommoninput
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
)

target_link_libraries(mircommoninput
  PUBLIC
    mircore
)
