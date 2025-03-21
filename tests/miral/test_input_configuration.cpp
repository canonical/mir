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
#include <tuple>

using Mouse = miral::InputConfiguration::Mouse;
using Keyboard = miral::InputConfiguration::Keyboard;
using Touchpad = miral::InputConfiguration::Touchpad;

struct TestInputConfiguration: testing::Test
{
    inline static auto const unclamped_test_values = {-1.0, -0.5, 0.0, 0.5, 1.0};
    inline static auto const clamped_test_values = {std::pair{-10.0, -1.0}, {-1.1, -1.0}, {1.1, 1.0}, {12.0, 1.0}};
};

TEST_F(TestInputConfiguration, mouse_acceleration_bias_is_set_and_clamped)
{
    Mouse mouse;
    ASSERT_FALSE(mouse.acceleration_bias().has_value());

    // No clamping in the range [-1, 1]
    for (auto const value : unclamped_test_values)
    {
        mouse.acceleration_bias(value);
        ASSERT_EQ(mouse.acceleration_bias(), value);
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        mouse.acceleration_bias(value);
        ASSERT_EQ(mouse.acceleration_bias(), expected);
    }
}

TEST_F(TestInputConfiguration, touchpad_acceleration_bias_is_set_and_clamped)
{
    Touchpad touch;
    ASSERT_FALSE(touch.acceleration_bias().has_value());

    for (auto const value : unclamped_test_values)
    {
        touch.acceleration_bias(value);
        ASSERT_EQ(touch.acceleration_bias(), value);
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        touch.acceleration_bias(value);
        ASSERT_EQ(touch.acceleration_bias(), expected);
    }
}

namespace miral
{
auto operator==(Mouse const& lhs, Mouse const& rhs) -> bool
{
    auto const left = std::tuple{
        lhs.handedness(), lhs.acceleration(), lhs.acceleration_bias(), lhs.vscroll_speed(), lhs.hscroll_speed()};
    auto const right = std::tuple{
        rhs.handedness(), rhs.acceleration(), rhs.acceleration_bias(), rhs.vscroll_speed(), rhs.hscroll_speed()};

    return left == right;
}
auto operator==(Touchpad const& lhs, Touchpad const& rhs) -> bool
{
    auto const left = std::tuple{
        lhs.acceleration(),
        lhs.acceleration_bias(),
        lhs.hscroll_speed(),
        lhs.vscroll_speed(),
        lhs.click_mode(),
        lhs.disable_while_typing(),
        lhs.disable_with_external_mouse(),
        lhs.middle_mouse_button_emulation(),
        lhs.scroll_mode(),
        lhs.tap_to_click()};

    auto const right = std::tuple{
        rhs.acceleration(),
        rhs.acceleration_bias(),
        rhs.hscroll_speed(),
        rhs.vscroll_speed(),
        rhs.click_mode(),
        rhs.disable_while_typing(),
        rhs.disable_with_external_mouse(),
        rhs.middle_mouse_button_emulation(),
        rhs.scroll_mode(),
        rhs.tap_to_click()};

    return left == right;
}
auto operator==(miral::InputConfiguration::Keyboard const& lhs, miral::InputConfiguration::Keyboard const& rhs) -> bool
{
    return std::tuple{lhs.repeat_rate(), rhs.repeat_delay()} == std::tuple{rhs.repeat_rate(), rhs.repeat_delay()};
}
}

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

// Make sure tht merging does not alter other data members
TEST_F(TestInputConfiguration, mouse_merge_does_not_overwrite_values)
{
    enum SettingToTest {
        handedness,
        acceleration,
        acceleration_bias,
        vscroll_speed,
        hscroll_speed,
    };

    std::initializer_list<std::pair<std::function<void(Mouse&)>, SettingToTest>> const setters = {
        {[](auto& mouse_config) { mouse_config.handedness(mir_pointer_handedness_right);        }, handedness},
        {[](auto& mouse_config) { mouse_config.acceleration(mir_pointer_acceleration_adaptive); }, acceleration},
        {[](auto& mouse_config) { mouse_config.acceleration_bias(-1);                           }, acceleration_bias},
        {[](auto& mouse_config) { mouse_config.vscroll_speed(2.0);                              }, vscroll_speed},
        {[](auto& mouse_config) { mouse_config.hscroll_speed(3.0);                              }, hscroll_speed}};

    auto const target_settings = [](auto& mouse_config, SettingToTest setting_to_test)
    {
        if (setting_to_test != handedness)
            mouse_config.handedness(mir_pointer_handedness_left);

        if (setting_to_test != acceleration)
            mouse_config.acceleration(mir_pointer_acceleration_none);

        if (setting_to_test != acceleration_bias)
            mouse_config.acceleration_bias(1);

        if (setting_to_test != vscroll_speed)
            mouse_config.vscroll_speed(3.0);

        if (setting_to_test != hscroll_speed)
            mouse_config.hscroll_speed(2.0);
    };

    for(auto const& [setter, setting_to_test]: setters)
    {
        // Initialize all members, overwrite the one we're testing
        Mouse expected;
        target_settings(expected, setting_to_test);
        setter(expected);

        // Set the value of member we're testing
        Mouse modified;
        setter(modified);

        // Initialize all members, _except_ the one we're testing
        Mouse target;
        target_settings(target, setting_to_test);

        // This merge should only modify the variable we're testing
        target.merge(modified);

        ASSERT_EQ(target, expected);
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

TEST_F(TestInputConfiguration, touchapd_merge_does_not_overwrite_values)
{
    enum SettingToTest {
        disable_while_typing,
        disable_with_external_mouse,
        acceleration,
        acceleration_bias,
        vscroll_speed,
        hscroll_speed,
        click_mode,
        scroll_mode,
        tap_to_click,
        middle_mouse_button_emulation
    };

    std::initializer_list<std::pair<std::function<void(Touchpad&)>, SettingToTest>> const setters = {
        {[](Touchpad& touch) { touch.disable_while_typing(true);                              }, disable_while_typing},
        {[](Touchpad& touch) { touch.disable_with_external_mouse(false);                      }, disable_with_external_mouse},
        {[](Touchpad& touch) { touch.acceleration(mir_pointer_acceleration_adaptive);         }, acceleration},
        {[](Touchpad& touch) { touch.acceleration_bias(1.0);                                  }, acceleration_bias},
        {[](Touchpad& touch) { touch.vscroll_speed(-13.0);                                    }, vscroll_speed},
        {[](Touchpad& touch) { touch.hscroll_speed(-0.4);                                     }, hscroll_speed},
        {[](Touchpad& touch) { touch.click_mode(mir_touchpad_click_mode_finger_count);        }, click_mode},
        {[](Touchpad& touch) { touch.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll); }, scroll_mode},
        {[](Touchpad& touch) { touch.tap_to_click(true);                                      }, tap_to_click},
        {[](Touchpad& touch) { touch.middle_mouse_button_emulation(false);                    }, middle_mouse_button_emulation}};

    auto const target_settings = [](auto& touchpad_config, SettingToTest setting_to_test)
    {
        if (setting_to_test != disable_while_typing)
            touchpad_config.disable_while_typing(false);

        if (setting_to_test != acceleration)
            touchpad_config.acceleration(mir_pointer_acceleration_none);

        if (setting_to_test != acceleration_bias)
            touchpad_config.acceleration_bias(1);

        if (setting_to_test != vscroll_speed)
            touchpad_config.vscroll_speed(3.0);

        if (setting_to_test != hscroll_speed)
            touchpad_config.hscroll_speed(2.0);

        if (setting_to_test != click_mode)
            touchpad_config.click_mode(mir_touchpad_click_mode_none);

        if (setting_to_test != scroll_mode)
            touchpad_config.scroll_mode(mir_touchpad_scroll_mode_none);

        if (setting_to_test != tap_to_click)
            touchpad_config.tap_to_click(false);

        if (setting_to_test != middle_mouse_button_emulation)
            touchpad_config.middle_mouse_button_emulation(true);
    };

    for(auto const& [setter, setting_to_test]: setters)
    {
        Touchpad expected;
        target_settings(expected, setting_to_test);
        setter(expected);

        Touchpad modified;
        setter(modified);

        Touchpad target;
        target_settings(target, setting_to_test);

        target.merge(modified);

        ASSERT_EQ(target, expected);
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

TEST_F(TestInputConfiguration, keyboard_merge_does_not_overwrite_values)
{
    enum SettingToTest { repeat_rate, repeat_delay };
    std::initializer_list<std::pair<std::function<void(Keyboard&)>, SettingToTest>> const setters = {
        {[](Keyboard& key)
         {
             key.set_repeat_rate(18);
         },
         repeat_rate},
        {[](Keyboard& key)
         {
             key.set_repeat_delay(1337);
         },
         repeat_delay},
    };

    auto const target_settings = [](auto& keyboard_config, SettingToTest setting_to_test)
    {
        if (setting_to_test != repeat_rate)
            keyboard_config.set_repeat_rate(4);

        if (setting_to_test != repeat_delay)
            keyboard_config.set_repeat_delay(500);
    };

    for(auto const& [setter, setting_to_test]: setters)
    {
        Keyboard expected;
        target_settings(expected, setting_to_test);
        setter(expected);

        Keyboard modified;
        setter(modified);

        Keyboard target;
        target_settings(target, setting_to_test);

        target.merge(modified);

        ASSERT_EQ(target, expected);
    }
}
