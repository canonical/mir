/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "input_device_config.h"
#include "mir_toolkit/mir_input_device_types.h"
#include "miral/input_configuration.h"
#include <gtest/gtest.h>

using Mouse = miral::InputConfiguration::Mouse;
using Keyboard = miral::InputConfiguration::Keyboard;
using Touchpad = miral::InputConfiguration::Touchpad;

struct TestInputConfiguration: testing::Test
{
};

// Make sure that when merging, each data member is assigned to the correct
// corresponding one.
//
// If each individual data member is merged correctly, then all combinations
// will work.
TEST_F(TestInputConfiguration, mouse_merge_from_partial_set_changes_only_set_values)
{
    std::initializer_list<std::function<void(Mouse&)>> const setters = {
        [](auto& mouse_config)
        {
            mouse_config.handedness(mir_pointer_handedness_right);
        },
        [](auto& mouse_config)
        {
            mouse_config.acceleration(mir_pointer_acceleration_adaptive);
        },
        [](auto& mouse_config)
        {
            mouse_config.acceleration_bias(-1);
        },
        [](auto& mouse_config)
        {
            mouse_config.vscroll_speed(2.0);
        },
        [](auto& mouse_config)
        {
            mouse_config.hscroll_speed(3.0);
        },
    };

    for(auto const& setter: setters)
    {
        Mouse expected;
        setter(expected);

        Mouse modified;
        setter(modified);

        Mouse merged;
        merged.merge(modified);
        ASSERT_EQ(merged, expected);
    }
}

TEST_F(TestInputConfiguration, touchpad_merge_from_partial_set_changes_only_set_values)
{
    std::initializer_list<std::function<void(Touchpad&)>> const setters = {
        [](Touchpad& touch)
        {
            touch.disable_while_typing(true);
        },
        [](Touchpad& touch)
        {
            touch.disable_with_external_mouse(false);
        },
        [](Touchpad& touch)
        {
            touch.acceleration(mir_pointer_acceleration_none);
        },
        [](Touchpad& touch)
        {
            touch.acceleration_bias(1.0);
        },
        [](Touchpad& touch)
        {
            touch.vscroll_speed(-13.0);
        },
        [](Touchpad& touch)
        {
            touch.hscroll_speed(-0.4);
        },
        [](Touchpad& touch)
        {
            touch.click_mode(mir_touchpad_click_mode_finger_count);
        },
        [](Touchpad& touch)
        {
            touch.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll);
        },
        [](Touchpad& touch)
        {
            touch.tap_to_click(true);
        },
        [](Touchpad& touch)
        {
            touch.middle_mouse_button_emulation(false);
        }};

    for(auto const& setter: setters)
    {
        Touchpad expected;
        setter(expected);

        Touchpad modified;
        setter(modified);

        Touchpad merged;
        merged.merge(modified);
        ASSERT_EQ(merged, expected);
    }
}

TEST_F(TestInputConfiguration, keyboard_merge_from_partial_set_changes_only_set_values)
{
    std::initializer_list<std::function<void(Keyboard&)>> const setters = {
        [](Keyboard& key)
        {
            key.set_repeat_rate(18);
        },
        [](Keyboard& key)
        {
            key.set_repeat_delay(1337);
        },
        };

    for(auto const& setter: setters)
    {
        Keyboard expected;
        setter(expected);

        Keyboard modified;
        setter(modified);

        Keyboard merged;
        merged.merge(modified);
        ASSERT_EQ(merged, expected);
    }
}
