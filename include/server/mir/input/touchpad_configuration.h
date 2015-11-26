/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_
#define MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device.h"

namespace mir
{
namespace input
{

struct TouchpadConfiguration
{
    TouchpadConfiguration() {}
    TouchpadConfiguration(MirTouchpadClickModes click_mode,
                          MirTouchpadScrollModes scroll_mode,
                          int button_down_scroll_button,
                          bool tap_to_click,
                          bool disable_while_typing,
                          bool disable_with_mouse,
                          bool middle_mouse_button_emulation)
        : click_mode{click_mode}, scroll_mode{scroll_mode}, button_down_scroll_button{button_down_scroll_button},
          tap_to_click{tap_to_click}, middle_mouse_button_emulation{middle_mouse_button_emulation},
          disable_with_mouse{disable_with_mouse}, disable_while_typing{disable_while_typing}
    {
    }

    /*!
     * The click mode defines when the touchpad generates software emulated button events.
     */
    MirTouchpadClickModes click_mode{mir_touchpad_click_mode_finger_count};
/*!
     * The scroll mode defines when the touchpad generates scroll events instead of pointer motion events.
     */
    MirTouchpadScrollModes scroll_mode{mir_touchpad_scroll_mode_two_finger_scroll};

    /*!
     * Configures the button used for the on-button-down scroll mode
     */
    int button_down_scroll_button{0};

    /*!
     * When tap to click is enabled the system will interpret short finger touch down/up sequences as button clicks.
     */
    bool tap_to_click{true};
    /*!
     * Emulates a middle mouse button press when the left and right buttons on a touchpad are pressed.
     */
    bool middle_mouse_button_emulation{true};
    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when another pointing device is plugged in.
     */
    bool disable_with_mouse{false};
    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when the user starts to use a keyboard and a short period after.
     */
    bool disable_while_typing{false};
};

}
}

#endif
