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

#include "mir/input/touchpad_configuration.h"

namespace mi = mir::input;

mi::TouchpadConfiguration::TouchpadConfiguration(MirTouchpadClickModes click_mode,
                                                 MirTouchpadScrollModes scroll_mode,
                                                 int button_down_scroll_button,
                                                 bool tap_to_click,
                                                 bool disable_while_typing,
                                                 bool disable_with_mouse,
                                                 bool middle_mouse_button_emulation) :
    click_mode_{click_mode}, scroll_mode_{scroll_mode}, button_down_scroll_button_{button_down_scroll_button},
    tap_to_click_{tap_to_click}, disable_while_typing_{disable_while_typing}, disable_with_mouse_{disable_with_mouse},
    middle_mouse_button_emulation_{middle_mouse_button_emulation}
{
}

mi::TouchpadConfiguration::TouchpadConfiguration() :
    click_mode_{mir_touchpad_click_mode_finger_count}, scroll_mode_{mir_touchpad_scroll_mode_two_finger_scroll}, button_down_scroll_button_{0}, tap_to_click_{true}, disable_while_typing_{false}, disable_with_mouse_{false}, middle_mouse_button_emulation_{true}
{
}

mi::TouchpadConfiguration::~TouchpadConfiguration()
{
}

void mi::TouchpadConfiguration::tap_to_click(bool enabled)
{
    tap_to_click_ = enabled;
}

bool mi::TouchpadConfiguration::tap_to_click() const
{
    return tap_to_click_;
}

void mi::TouchpadConfiguration::middle_mouse_button_emulation(bool enabled)
{
    middle_mouse_button_emulation_ = enabled;
}

bool mi::TouchpadConfiguration::middle_mouse_button_emulation() const
{
    return middle_mouse_button_emulation_;
}

void mi::TouchpadConfiguration::disable_with_mouse(bool enabled)
{
    disable_with_mouse_ = enabled;
}

bool mi::TouchpadConfiguration::disable_with_mouse() const
{
    return disable_with_mouse_;
}

void mi::TouchpadConfiguration::disable_while_typing(bool enabled)
{
    disable_while_typing_ = enabled;
}

bool mi::TouchpadConfiguration::disable_while_typing() const
{
    return disable_while_typing_;
}

void mi::TouchpadConfiguration::click_mode(MirTouchpadClickModes click_mode)
{
    click_mode_ = click_mode;
}

MirTouchpadClickModes mi::TouchpadConfiguration::click_mode() const
{
    return click_mode_;
}

void mi::TouchpadConfiguration::scroll_mode(MirTouchpadScrollModes scroll_mode)
{
    scroll_mode_ = scroll_mode;
}

MirTouchpadScrollModes mi::TouchpadConfiguration::scroll_mode() const
{
    return scroll_mode_;
}

void mi::TouchpadConfiguration::scroll_mode(int scroll_button)
{
    scroll_mode_ |= mir_touchpad_scroll_mode_button_down_scroll;
    button_down_scroll_button_ = scroll_button;
}

int mi::TouchpadConfiguration::scroll_button() const
{
    return button_down_scroll_button_;
}

