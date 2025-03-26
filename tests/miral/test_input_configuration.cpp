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

#include "mir_toolkit/mir_input_device_types.h"
#include "miral/input_configuration.h"

#include <functional>
#include <tuple>

#include <gtest/gtest.h>

using Mouse = miral::InputConfiguration::Mouse;
using Keyboard = miral::InputConfiguration::Keyboard;
using Touchpad = miral::InputConfiguration::Touchpad;

template <typename T> using PropertySetter = std::function<void(T&)>;

enum class MouseProperty
{
    handedness,
    acceleration,
    acceleration_bias,
    vscroll_speed,
    hscroll_speed
};

enum class TouchpadProperty
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

enum class KeyboardProperty
{
    repeat_rate,
    repeat_delay
};

struct TestInputConfiguration: testing::Test
{
    inline static auto const unclamped_test_values = {-1.0, -0.5, 0.0, 0.5, 1.0};
    inline static auto const clamped_test_values = {std::pair{-10.0, -1.0}, {-1.1, -1.0}, {1.1, 1.0}, {12.0, 1.0}};

    inline static std::unordered_map<MouseProperty, PropertySetter<Mouse>> mouse_setters = {
        {
            MouseProperty::handedness,
            [](auto& mouse_config)
            {
                mouse_config.handedness(mir_pointer_handedness_right);
            },
        },
        {
            MouseProperty::acceleration,
            [](auto& mouse_config)
            {
                mouse_config.acceleration(mir_pointer_acceleration_adaptive);
            },
        },
        {
            MouseProperty::acceleration_bias,
            [](auto& mouse_config)
            {
                mouse_config.acceleration_bias(-1);
            },
        },
        {
            MouseProperty::vscroll_speed,
            [](auto& mouse_config)
            {
                mouse_config.vscroll_speed(2.0);
            },
        },
        {
            MouseProperty::hscroll_speed,
            [](auto& mouse_config)
            {
                mouse_config.hscroll_speed(3.0);
            },
        },
    };

    inline static std::unordered_map<TouchpadProperty, PropertySetter<Touchpad>> const touchpad_setters = {
        {
            TouchpadProperty::disable_while_typing,
            [](Touchpad& touch)
            {
                touch.disable_while_typing(true);
            },
        },
        {
            TouchpadProperty::disable_with_external_mouse,
            [](Touchpad& touch)
            {
                touch.disable_with_external_mouse(false);
            },
        },
        {
            TouchpadProperty::acceleration,
            [](Touchpad& touch)
            {
                touch.acceleration(mir_pointer_acceleration_none);
            },
        },
        {
            TouchpadProperty::acceleration_bias,
            [](Touchpad& touch)
            {
                touch.acceleration_bias(1.0);
            },
        },
        {
            TouchpadProperty::vscroll_speed,
            [](Touchpad& touch)
            {
                touch.vscroll_speed(-13.0);
            },
        },
        {
            TouchpadProperty::hscroll_speed,
            [](Touchpad& touch)
            {
                touch.hscroll_speed(-0.4);
            },
        },
        {
            TouchpadProperty::click_mode,
            [](Touchpad& touch)
            {
                touch.click_mode(mir_touchpad_click_mode_finger_count);
            },
        },
        {
            TouchpadProperty::scroll_mode,
            [](Touchpad& touch)
            {
                touch.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll);
            },
        },
        {
            TouchpadProperty::tap_to_click,
            [](Touchpad& touch)
            {
                touch.tap_to_click(true);
            },
        },
        {
            TouchpadProperty::middle_mouse_button_emulation,
            [](Touchpad& touch)
            {
                touch.middle_mouse_button_emulation(false);
            },
        },
    };

    inline static std::unordered_map<KeyboardProperty, PropertySetter<Keyboard>> const keyboard_setters = {
        {
            KeyboardProperty::repeat_rate,
            [](Keyboard& key)
            {
                key.set_repeat_rate(18);
            },
        },
        {
            KeyboardProperty::repeat_delay,
            [](Keyboard& key)
            {
                key.set_repeat_delay(1337);
            },
        }};
};

TEST_F(TestInputConfiguration, mouse_acceleration_bias_is_set_and_clamped)
{
    Mouse mouse;
    ASSERT_FALSE(mouse.acceleration_bias().has_value());

    // No clamping in the range [-1, 1]
    for (auto const value : unclamped_test_values)
    {
        mouse.acceleration_bias(value);
        EXPECT_EQ(mouse.acceleration_bias(), value);
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        mouse.acceleration_bias(value);
        EXPECT_EQ(mouse.acceleration_bias(), expected);
    }
}

TEST_F(TestInputConfiguration, touchpad_acceleration_bias_is_set_and_clamped)
{
    Touchpad touch;
    ASSERT_FALSE(touch.acceleration_bias().has_value());

    for (auto const value : unclamped_test_values)
    {
        touch.acceleration_bias(value);
        EXPECT_EQ(touch.acceleration_bias(), value);
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        touch.acceleration_bias(value);
        EXPECT_EQ(touch.acceleration_bias(), expected);
    }
}

namespace
{
void mouse_expect_equal(Mouse const& lhs, Mouse const& rhs)
{
    EXPECT_EQ(lhs.handedness(), rhs.handedness());
    EXPECT_EQ(lhs.acceleration(), rhs.acceleration());
    EXPECT_EQ(lhs.acceleration_bias(), rhs.acceleration_bias());
    EXPECT_EQ(lhs.vscroll_speed(), rhs.vscroll_speed());
    EXPECT_EQ(lhs.hscroll_speed(), rhs.hscroll_speed());
}

auto mouse_equal(Mouse const& lhs, Mouse const& rhs) -> bool
{
    mouse_expect_equal(lhs, rhs);

    auto const left = std::tuple{
        lhs.handedness(), lhs.acceleration(), lhs.acceleration_bias(), lhs.vscroll_speed(), lhs.hscroll_speed()};
    auto const right = std::tuple{
        rhs.handedness(), rhs.acceleration(), rhs.acceleration_bias(), rhs.vscroll_speed(), rhs.hscroll_speed()};

    return left == right;
}

void touchpad_expect_equal(Touchpad const& lhs, Touchpad const& rhs)
{
    EXPECT_EQ(lhs.acceleration(), rhs.acceleration());
    EXPECT_EQ(lhs.acceleration_bias(), rhs.acceleration_bias());
    EXPECT_EQ(lhs.hscroll_speed(), rhs.hscroll_speed());
    EXPECT_EQ(lhs.vscroll_speed(), rhs.vscroll_speed());
    EXPECT_EQ(lhs.click_mode(), rhs.click_mode());
    EXPECT_EQ(lhs.disable_while_typing(), rhs.disable_while_typing());
    EXPECT_EQ(lhs.disable_with_external_mouse(), rhs.disable_with_external_mouse());
    EXPECT_EQ(lhs.middle_mouse_button_emulation(), rhs.middle_mouse_button_emulation());
    EXPECT_EQ(lhs.scroll_mode(), rhs.scroll_mode());
    EXPECT_EQ(lhs.tap_to_click(), rhs.tap_to_click());
}

auto touchpad_equal(Touchpad const& lhs, Touchpad const& rhs) -> bool
{
    touchpad_expect_equal(lhs, rhs);

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

void keyboard_expect_equal(Keyboard const& lhs, Keyboard const& rhs)
{
    EXPECT_EQ(lhs.repeat_rate(), rhs.repeat_rate());
    EXPECT_EQ(lhs.repeat_delay(), rhs.repeat_delay());
}

auto keyboard_equal(Keyboard const& lhs, Keyboard const& rhs) -> bool
{
    keyboard_expect_equal(lhs, rhs);
    return std::tuple{lhs.repeat_rate(), rhs.repeat_delay()} == std::tuple{rhs.repeat_rate(), rhs.repeat_delay()};
}
}

TEST_F(TestInputConfiguration, mouse_getters_return_expected_values)
{
    {
        Mouse mouse_config;
        auto const value = mir_pointer_acceleration_adaptive;
        mouse_config.acceleration(value);
        EXPECT_EQ(mouse_config.acceleration(), value);
    }

    {
        Mouse mouse_config;
        auto const value = -1;
        mouse_config.acceleration_bias(value);
        EXPECT_EQ(mouse_config.acceleration_bias(), value);
    }

    {
        Mouse mouse_config;
        auto const value = mir_pointer_handedness_right;
        mouse_config.handedness(value);
        EXPECT_EQ(mouse_config.handedness(), value);
    }

    {
        Mouse mouse_config;
        auto const value = -5;
        mouse_config.hscroll_speed(value);
        EXPECT_EQ(mouse_config.hscroll_speed(), value);
    }

    {
        Mouse mouse_config;
        auto const value = 5;
        mouse_config.vscroll_speed(value);
        EXPECT_EQ(mouse_config.vscroll_speed(), value);
    }
}

struct TestMouseConfiguration : public TestInputConfiguration, testing::WithParamInterface<MouseProperty>
{
};

// Make sure that when merging, each data member is assigned to the correct
// corresponding one.
//
// If each individual data member is merged correctly, then all combinations
// will work.
TEST_P(TestMouseConfiguration, mouse_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();
    auto const setter = mouse_setters.at(property);

    Mouse expected;
    setter(expected);

    Mouse modified;
    setter(modified);

    Mouse merged;
    merged.merge(modified);
    EXPECT_PRED2(mouse_equal, merged, expected);
}

// Make sure tht merging does not alter other data members
TEST_P(TestMouseConfiguration, mouse_merge_does_not_overwrite_values)
{
    // Target settings must be different from the values set by setters at the top of the file.
    auto const target_settings = [](auto& mouse_config, MouseProperty property)
    {
        if (property != MouseProperty::handedness)
            mouse_config.handedness(mir_pointer_handedness_left);

        if (property != MouseProperty::acceleration)
            mouse_config.acceleration(mir_pointer_acceleration_none);

        if (property != MouseProperty::acceleration_bias)
            mouse_config.acceleration_bias(1);

        if (property != MouseProperty::vscroll_speed)
            mouse_config.vscroll_speed(3.0);

        if (property != MouseProperty::hscroll_speed)
            mouse_config.hscroll_speed(2.0);
    };

    auto const property = GetParam();
    auto const setter = mouse_setters.at(property);

    // Initialize all members, overwrite the one we're testing
    Mouse expected;
    target_settings(expected, property);
    setter(expected);

    // Set the value of member we're testing
    Mouse modified;
    setter(modified);

    // Initialize all members, _except_ the one we're testing
    Mouse target;
    target_settings(target, property);

    // This merge should only modify the variable we're testing
    target.merge(modified);

    EXPECT_PRED2(mouse_equal, target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration,
    TestMouseConfiguration,
    ::testing::Values(
        MouseProperty::handedness,
        MouseProperty::acceleration,
        MouseProperty::acceleration_bias,
        MouseProperty::vscroll_speed,
        MouseProperty::hscroll_speed));

TEST_F(TestInputConfiguration, touchpad_getters_return_expected_values)
{
    {
        Touchpad touch_config;
        auto const value = false;
        touch_config.disable_while_typing(value);
        EXPECT_EQ(touch_config.disable_while_typing(), value);
    }

    {
        Touchpad touch_config;
        auto const value = true;
        touch_config.disable_with_external_mouse(value);
        EXPECT_EQ(touch_config.disable_with_external_mouse(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_pointer_acceleration_adaptive;
        touch_config.acceleration(value);
        EXPECT_EQ(touch_config.acceleration(), value);
    }

    {
        Touchpad touch_config;
        auto const value = -1;
        touch_config.acceleration_bias(value);
        EXPECT_EQ(touch_config.acceleration_bias(), value);
    }

    {
        Touchpad touch_config;
        auto const value = -5;
        touch_config.hscroll_speed(value);
        EXPECT_EQ(touch_config.hscroll_speed(), value);
    }

    {
        Touchpad touch_config;
        auto const value = 5;
        touch_config.vscroll_speed(value);
        EXPECT_EQ(touch_config.vscroll_speed(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_touchpad_click_mode_area_to_click;
        touch_config.click_mode(value);
        EXPECT_EQ(touch_config.click_mode(), value);
    }

    {
        Touchpad touch_config;
        auto const value = mir_touchpad_scroll_mode_edge_scroll;
        touch_config.scroll_mode(value);
        EXPECT_EQ(touch_config.scroll_mode(), value);
    }

    {
        Touchpad touch_config;
        auto const value = false;
        touch_config.tap_to_click(value);
        EXPECT_EQ(touch_config.tap_to_click(), value);
    }

    {
        Touchpad touch_config;
        auto const value = true;
        touch_config.middle_mouse_button_emulation(value);
        EXPECT_EQ(touch_config.middle_mouse_button_emulation(), value);
    }
}

struct TestTouchpadConfiguration : public TestInputConfiguration, testing::WithParamInterface<TouchpadProperty>
{
};

TEST_P(TestTouchpadConfiguration, touchpad_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();
    auto const setter = touchpad_setters.at(property);

    Touchpad expected;
    setter(expected);

    Touchpad modified;
    setter(modified);

    Touchpad merged;
    merged.merge(modified);
    EXPECT_PRED2(touchpad_equal, merged, expected);
}

TEST_P(TestTouchpadConfiguration, touchapd_merge_does_not_overwrite_values)
{
    auto const target_settings = [](auto& touchpad_config, TouchpadProperty property)
    {
        if (property != TouchpadProperty::disable_while_typing)
            touchpad_config.disable_while_typing(false);

        if (property != TouchpadProperty::acceleration)
            touchpad_config.acceleration(mir_pointer_acceleration_none);

        if (property != TouchpadProperty::acceleration_bias)
            touchpad_config.acceleration_bias(1);

        if (property != TouchpadProperty::vscroll_speed)
            touchpad_config.vscroll_speed(3.0);

        if (property != TouchpadProperty::hscroll_speed)
            touchpad_config.hscroll_speed(2.0);

        if (property != TouchpadProperty::click_mode)
            touchpad_config.click_mode(mir_touchpad_click_mode_none);

        if (property != TouchpadProperty::scroll_mode)
            touchpad_config.scroll_mode(mir_touchpad_scroll_mode_none);

        if (property != TouchpadProperty::tap_to_click)
            touchpad_config.tap_to_click(false);

        if (property != TouchpadProperty::middle_mouse_button_emulation)
            touchpad_config.middle_mouse_button_emulation(true);
    };

    auto const property = GetParam();
    auto const setter = touchpad_setters.at(property);

    Touchpad expected;
    target_settings(expected, property);
    setter(expected);

    Touchpad modified;
    setter(modified);

    Touchpad target;
    target_settings(target, property);

    target.merge(modified);

    EXPECT_PRED2(touchpad_equal, target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration,
    TestTouchpadConfiguration,
    ::testing::Values(
        TouchpadProperty::disable_while_typing,
        TouchpadProperty::disable_with_external_mouse,
        TouchpadProperty::acceleration,
        TouchpadProperty::acceleration_bias,
        TouchpadProperty::vscroll_speed,
        TouchpadProperty::hscroll_speed,
        TouchpadProperty::click_mode,
        TouchpadProperty::scroll_mode,
        TouchpadProperty::tap_to_click,
        TouchpadProperty::middle_mouse_button_emulation));

TEST_F(TestInputConfiguration, keyboard_getters_return_expected_values)
{
    {
        Keyboard keyboard_config;
        auto const value = 731;
        keyboard_config.set_repeat_rate(value);
        EXPECT_EQ(keyboard_config.repeat_rate(), value);
    }

    {
        Keyboard keyboard_config;
        auto const value = 2371;
        keyboard_config.set_repeat_delay(value);
        EXPECT_EQ(keyboard_config.repeat_delay(), value);
    }
}

struct TestKeyboardConfiguration : public TestInputConfiguration, testing::WithParamInterface<KeyboardProperty>
{
};

TEST_P(TestKeyboardConfiguration, keyboard_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();
    auto const setter = keyboard_setters.at(property);

    Keyboard expected;
    setter(expected);

    Keyboard modified;
    setter(modified);

    Keyboard merged;
    merged.merge(modified);
    EXPECT_PRED2(keyboard_equal, merged, expected);
}

TEST_P(TestKeyboardConfiguration, keyboard_merge_does_not_overwrite_values)
{
    auto const target_settings = [](auto& keyboard_config, KeyboardProperty property)
    {
        if (property != KeyboardProperty::repeat_rate)
            keyboard_config.set_repeat_rate(4);

        if (property != KeyboardProperty::repeat_delay)
            keyboard_config.set_repeat_delay(500);
    };

    auto const property = GetParam();
    auto const setter = keyboard_setters.at(property);

    Keyboard expected;
    target_settings(expected, property);
    setter(expected);

    Keyboard modified;
    setter(modified);

    Keyboard target;
    target_settings(target, property);

    target.merge(modified);

    EXPECT_PRED2(keyboard_equal, target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration,
    TestKeyboardConfiguration,
    ::testing::Values(KeyboardProperty::repeat_rate, KeyboardProperty::repeat_delay));
