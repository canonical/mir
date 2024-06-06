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

    bool operator==(TouchpadSettings const& rhs) const = default;
    bool operator!=(TouchpadSettings const& rhs) const = default;

    MirTouchpadClickMode click_mode{mir_touchpad_click_mode_finger_count};
    MirTouchpadScrollMode scroll_mode{mir_touchpad_scroll_mode_two_finger_scroll};
    int button_down_scroll_button{no_scroll_button};
    bool tap_to_click{true};
    bool disable_while_typing{false};
    bool disable_with_mouse{false};
    bool middle_mouse_button_emulation{true};
};
}
}

#endif
