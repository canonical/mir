/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_TOUCH_PAD_SETTINGS_H_
#define MIR_INPUT_TOUCH_PAD_SETTINGS_H_

#include "mir_toolkit/mir_input_device.h"

namespace mir
{
namespace input
{
const int no_scroll_button = 0;

struct TouchPadSettings
{
    TouchPadSettings() {}
    MirTouchPadClickMode click_mode{mir_touch_pad_click_mode_finger_count};
    MirTouchPadScrollMode scroll_mode{mir_touch_pad_scroll_mode_two_finger_scroll};
    int button_down_scroll_button{no_scroll_button};
    bool tap_to_click{true};
    bool disable_while_typing{false};
    bool disable_with_mouse{false};
    bool middle_mouse_button_emulation{true};
};

}
}

#endif
