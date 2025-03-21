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
#include <functional>
#include <gtest/gtest.h>
#include <ranges>
#include <tuple>

using Mouse = miral::InputConfiguration::Mouse;
using Keyboard = miral::InputConfiguration::Keyboard;
using Touchpad = miral::InputConfiguration::Touchpad;

struct TestInputConfiguration: testing::Test
{
    inline static auto const unclamped_test_values = {-1.0, -0.5, 0.0, 0.5, 1.0};
    inline static auto const clamped_test_values = {std::pair{-10.0, -1.0}, {-1.1, -1.0}, {1.1, 1.0}, {12.0, 1.0}};


    inline static std::initializer_list<std::function<void(Mouse&)>> const mouse_setters = {
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

    inline static std::initializer_list<std::function<void(Touchpad&)>> const touchpad_setters = {
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

    inline static std::initializer_list<std::function<void(Keyboard&)>> const keyboard_setters = {
        [](Keyboard& key)
        {
            key.set_repeat_rate(18);
        },
        [](Keyboard& key)
        {
            key.set_repeat_delay(1337);
        },
    };
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

TEST_F(TestInputConfiguration, mouse_getters_return_expected_values)
{
    {
        Mouse mouse_config;
        auto const value = mir_pointer_acceleration_adaptive;
        mouse_config.acceleration(value);
        ASSERT_EQ(mouse_config.acceleration(), value);
    }

    {
        Mouse mouse_config;
        auto const value = -1;
        mouse_config.acceleration_bias(value);
        ASSERT_EQ(mouse_config.acceleration_bias(), value);
    }

    {
        Mouse mouse_config;
        auto const value = mir_pointer_handedness_right;
        mouse_config.handedness(value);
        ASSERT_EQ(mouse_config.handedness(), value);
    }

    {
        Mouse mouse_config;
        auto const value = -5;
        mouse_config.hscroll_speed(value);
        ASSERT_EQ(mouse_config.hscroll_speed(), value);
    }

    {
        Mouse mouse_config;
        auto const value = 5;
        mouse_config.vscroll_speed(value);
        ASSERT_EQ(mouse_config.vscroll_speed(), value);
    }
}

// Make sure that when merging, each data member is assigned to the correct
// corresponding one.
//
// If each individual data member is merged correctly, then all combinations
// will work.
TEST_F(TestInputConfiguration, mouse_merge_from_partial_set_changes_only_set_values)
{
    for(auto const& setter: mouse_setters)
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
    enum SettingToTest
    {
        handedness,
        acceleration,
        acceleration_bias,
        vscroll_speed,
        hscroll_speed
    };

    // Must be in the same order as the setters defined at the top of the file
    auto const settings_to_test = {handedness, acceleration, acceleration_bias, vscroll_speed, hscroll_speed};

    // Target settings must be different from the values set by setters at the top of the file.
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

    for (auto const& [setter, setting_to_test] : std::ranges::zip_view(mouse_setters, settings_to_test))
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

TEST_F(TestInputConfiguration, touchpad_getters_return_expected_values)
{
    {
        Touchpad touch_config;
        auto const value = false;
        touch_config.disable_while_typing(value);
        ASSERT_EQ(touch_config.disable_while_typing(), value);
    }

    {
        Touchpad touch_config;
        auto const value = true;
        touch_config.disable_with_external_mouse(value);
        ASSERT_EQ(touch_config.disable_with_external_mouse(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_pointer_acceleration_adaptive;
        touch_config.acceleration(value);
        ASSERT_EQ(touch_config.acceleration(), value);
    }

    {
        Touchpad touch_config;
        auto const value = -1;
        touch_config.acceleration_bias(value);
        ASSERT_EQ(touch_config.acceleration_bias(), value);
    }

    {
        Touchpad touch_config;
        auto const value = -5;
        touch_config.hscroll_speed(value);
        ASSERT_EQ(touch_config.hscroll_speed(), value);
    }

    {
        Touchpad touch_config;
        auto const value = 5;
        touch_config.vscroll_speed(value);
        ASSERT_EQ(touch_config.vscroll_speed(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_touchpad_click_mode_area_to_click;
        touch_config.click_mode(value);
        ASSERT_EQ(touch_config.click_mode(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_touchpad_scroll_mode_edge_scroll;
        touch_config.scroll_mode(value);
        ASSERT_EQ(touch_config.scroll_mode(), value);
    }

    {
        Touchpad touch_config;
        auto const value = false;
        touch_config.tap_to_click(value);
        ASSERT_EQ(touch_config.tap_to_click(), value);
    }

    {
        Touchpad touch_config;
        auto const value = true;
        touch_config.middle_mouse_button_emulation(value);
        ASSERT_EQ(touch_config.middle_mouse_button_emulation(), value);
    }
}

TEST_F(TestInputConfiguration, touchpad_merge_from_partial_set_changes_only_set_values)
{
    for(auto const& setter: touchpad_setters)
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
    enum SettingToTest
    {
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

    auto const settings_to_test = {
        disable_while_typing,
        disable_with_external_mouse,
        acceleration,
        acceleration_bias,
        vscroll_speed,
        hscroll_speed,
        click_mode,
        scroll_mode,
        tap_to_click,
        middle_mouse_button_emulation,
    };

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

    for (auto const& [setter, setting_to_test] : std::ranges::zip_view(touchpad_setters, settings_to_test))
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


TEST_F(TestInputConfiguration, keyboard_getters_return_expected_values)
{
    {
        Keyboard keyboard_config;
        auto const value = 731;
        keyboard_config.set_repeat_rate(value);
        ASSERT_EQ(keyboard_config.repeat_rate(), value);
    }

    {
        Keyboard keyboard_config;
        auto const value = 2371;
        keyboard_config.set_repeat_delay(value);
        ASSERT_EQ(keyboard_config.repeat_delay(), value);
    }
}


TEST_F(TestInputConfiguration, keyboard_merge_from_partial_set_changes_only_set_values)
{
    for(auto const& setter: keyboard_setters)
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
    auto const settings_to_test = { repeat_rate, repeat_delay  };

    auto const target_settings = [](auto& keyboard_config, SettingToTest setting_to_test)
    {
        if (setting_to_test != repeat_rate)
            keyboard_config.set_repeat_rate(4);

        if (setting_to_test != repeat_delay)
            keyboard_config.set_repeat_delay(500);
    };

    for (auto const& [setter, setting_to_test] : std::ranges::zip_view(keyboard_setters, settings_to_test))
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
