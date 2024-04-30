/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_TOUCH_PAD_SETTINGS_H_
#define MIR_INPUT_TOUCH_PAD_SETTINGS_H_

#include "mir_toolkit/mir_input_device_types.h"

namespace mir
{
namespace input
{
int const no_scroll_button = 0;

struct TouchpadSettings
{
    TouchpadSettings() {}
    MirTouchpadClickModes click_mode{mir_touchpad_click_mode_finger_count};
    MirTouchpadScrollModes scroll_mode{mir_touchpad_scroll_mode_two_finger_scroll};
    int button_down_scroll_button{no_scroll_button};
    bool tap_to_click{true};
    bool disable_while_typing{false};
    bool disable_with_mouse{false};
    bool middle_mouse_button_emulation{true};
};

inline bool operator==(TouchpadSettings const& lhs, TouchpadSettings const& rhs)
{
    return lhs.click_mode == rhs.click_mode && lhs.scroll_mode == rhs.scroll_mode &&
           lhs.button_down_scroll_button == rhs.button_down_scroll_button && lhs.tap_to_click == rhs.tap_to_click &&
           lhs.disable_while_typing == rhs.disable_while_typing && lhs.disable_with_mouse == rhs.disable_with_mouse &&
           lhs.middle_mouse_button_emulation == rhs.middle_mouse_button_emulation;
}

}
}

#endif
