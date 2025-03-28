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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <functional>
#include <span>

using namespace testing;

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
static auto const all_mouse_props = {
    MouseProperty::handedness,
    MouseProperty::acceleration,
    MouseProperty::acceleration_bias,
    MouseProperty::vscroll_speed,
    MouseProperty::hscroll_speed,
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
static auto const all_touchpad_props = {
    TouchpadProperty::disable_while_typing,
    TouchpadProperty::disable_with_external_mouse,
    TouchpadProperty::acceleration,
    TouchpadProperty::acceleration_bias,
    TouchpadProperty::vscroll_speed,
    TouchpadProperty::hscroll_speed,
    TouchpadProperty::click_mode,
    TouchpadProperty::scroll_mode,
    TouchpadProperty::tap_to_click,
    TouchpadProperty::middle_mouse_button_emulation};

enum class KeyboardProperty
{
    repeat_rate,
    repeat_delay
};
static auto const all_keyboard_props = {KeyboardProperty::repeat_rate, KeyboardProperty::repeat_delay};

struct TestInputConfiguration: testing::Test
{
    inline static auto const unclamped_test_values = {-1.0, -0.5, 0.0, 0.5, 1.0};
    inline static auto const clamped_test_values = {std::pair{-10.0, -1.0}, {-1.1, -1.0}, {1.1, 1.0}, {12.0, 1.0}};

    inline static std::unordered_map<MouseProperty, PropertySetter<Mouse>> const mouse_setters = {
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

namespace
{
auto get_mouse_config_with_properties(std::span<const MouseProperty> properties) -> Mouse
{
    Mouse mouse_config;
    for (auto const property : properties)
    {
        auto const property_setter = TestInputConfiguration::mouse_setters.at(property);
        property_setter(mouse_config);
    }

    return mouse_config;
}

auto get_mouse_config_with_all_properties_except(MouseProperty property) -> Mouse
{
    std::vector<MouseProperty> all_props{all_mouse_props.begin(), all_mouse_props.end()};
    all_props.erase(std::remove(all_props.begin(), all_props.end(), property));
    return get_mouse_config_with_properties(all_props);
}

auto get_touchpad_config_with_properties(std::span<const TouchpadProperty> properties) -> Touchpad
{
    Touchpad touchpad_config;
    for (auto const property : properties)
    {
        auto const property_setter = TestInputConfiguration::touchpad_setters.at(property);
        property_setter(touchpad_config);
    }

    return touchpad_config;
}

auto get_touchpad_config_with_all_properties_except(TouchpadProperty property) -> Touchpad
{
    std::vector<TouchpadProperty> all_props{all_touchpad_props.begin(), all_touchpad_props.end()};
    all_props.erase(std::remove(all_props.begin(), all_props.end(), property));
    return get_touchpad_config_with_properties(all_props);
}

auto get_keyboard_config_with_properties(std::span<const KeyboardProperty> properties) -> Keyboard
{
    Keyboard keyboard_config;
    for (auto const property : properties)
    {
        auto const property_setter = TestInputConfiguration::keyboard_setters.at(property);
        property_setter(keyboard_config);
    }

    return keyboard_config;
}

auto get_keyboard_config_with_all_properties_except(KeyboardProperty property) -> Keyboard
{
    std::vector<KeyboardProperty> all_props{all_keyboard_props.begin(), all_keyboard_props.end()};
    all_props.erase(std::remove(all_props.begin(), all_props.end(), property));
    return get_keyboard_config_with_properties(all_props);
}
}

TEST_F(TestInputConfiguration, mouse_acceleration_bias_is_set_and_clamped)
{
    Mouse mouse;
    ASSERT_THAT(mouse.acceleration_bias(), Eq(std::nullopt));

    // No clamping in the range [-1, 1]
    for (auto const value : unclamped_test_values)
    {
        mouse.acceleration_bias(value);
        EXPECT_THAT(mouse.acceleration_bias(), Eq(value));
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        mouse.acceleration_bias(value);
        EXPECT_THAT(mouse.acceleration_bias(), Eq(expected));
    }
}

TEST_F(TestInputConfiguration, touchpad_acceleration_bias_is_set_and_clamped)
{
    Touchpad touch;
    ASSERT_THAT(touch.acceleration_bias(), Eq(std::nullopt));

    for (auto const value : unclamped_test_values)
    {
        touch.acceleration_bias(value);
        EXPECT_THAT(touch.acceleration_bias(), Eq(value));
    }
    for (auto const& [value, expected] : clamped_test_values)
    {
        touch.acceleration_bias(value);
        EXPECT_THAT(touch.acceleration_bias(), Eq(expected));
    }
}

namespace
{
void mouse_expect_equal(Mouse const& actual, Mouse const& expected)
{
    EXPECT_THAT(actual.handedness(), Eq(expected.handedness()));
    EXPECT_THAT(actual.acceleration(), Eq(expected.acceleration()));
    EXPECT_THAT(actual.acceleration_bias(), Eq(expected.acceleration_bias()));
    EXPECT_THAT(actual.vscroll_speed(), Eq(expected.vscroll_speed()));
    EXPECT_THAT(actual.hscroll_speed(), Eq(expected.hscroll_speed()));
}

void touchpad_expect_equal(Touchpad const& actual, Touchpad const& expected)
{
    EXPECT_THAT(actual.acceleration(), Eq(expected.acceleration()));
    EXPECT_THAT(actual.acceleration_bias(), Eq(expected.acceleration_bias()));
    EXPECT_THAT(actual.hscroll_speed(), Eq(expected.hscroll_speed()));
    EXPECT_THAT(actual.vscroll_speed(), Eq(expected.vscroll_speed()));
    EXPECT_THAT(actual.click_mode(), Eq(expected.click_mode()));
    EXPECT_THAT(actual.disable_while_typing(), Eq(expected.disable_while_typing()));
    EXPECT_THAT(actual.disable_with_external_mouse(), Eq(expected.disable_with_external_mouse()));
    EXPECT_THAT(actual.middle_mouse_button_emulation(), Eq(expected.middle_mouse_button_emulation()));
    EXPECT_THAT(actual.scroll_mode(), Eq(expected.scroll_mode()));
    EXPECT_THAT(actual.tap_to_click(), Eq(expected.tap_to_click()));
}

void keyboard_expect_equal(Keyboard const& lhs, Keyboard const& rhs)
{
    EXPECT_THAT(lhs.repeat_rate(), Eq(rhs.repeat_rate()));
    EXPECT_THAT(lhs.repeat_delay(), Eq(rhs.repeat_delay()));
}
}

struct TestMouseInputConfiguration: public TestInputConfiguration
{
    Mouse mouse_config;
};

TEST_F(TestMouseInputConfiguration, mouse_acceleration_returns_expected_value)
{
    std::optional<MirPointerAcceleration> const values[] = {
        mir_pointer_acceleration_none, mir_pointer_acceleration_adaptive, std::nullopt};
    for (auto const value : values)
    {
        mouse_config.acceleration(value);
        EXPECT_THAT(mouse_config.acceleration(), Eq(value));
    }
}

TEST_F(TestMouseInputConfiguration, mouse_acceleration_bias_returns_expected_value)
{
    auto const value = -1;
    mouse_config.acceleration_bias(value);
    EXPECT_THAT(mouse_config.acceleration_bias(), Eq(value));
}

TEST_F(TestMouseInputConfiguration, mouse_handedness_returns_expected_value)
{
    auto const values = {mir_pointer_handedness_left, mir_pointer_handedness_right};
    for(auto const value: values)
    {
        mouse_config.handedness(value);
        EXPECT_THAT(mouse_config.handedness(), Eq(value));
    }
}

TEST_F(TestMouseInputConfiguration, mouse_hscroll_speed_returns_expected_value)
{
    auto const value = -5;
    mouse_config.hscroll_speed(value);
    EXPECT_THAT(mouse_config.hscroll_speed(), Eq(value));
}

TEST_F(TestMouseInputConfiguration, mouse_vscroll_speed_returns_expected_value)
{
    auto const value = 5;
    mouse_config.vscroll_speed(value);
    EXPECT_THAT(mouse_config.vscroll_speed(), Eq(value));
}

struct TestMouseMergeOneProperty : public TestInputConfiguration, testing::WithParamInterface<MouseProperty>
{
    Mouse target;
};

// Make sure that when merging, each data member is assigned to the correct
// corresponding one.
//
// If each individual data member is merged correctly, then all combinations
// will work.
TEST_P(TestMouseMergeOneProperty, mouse_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();

    auto const expected = get_mouse_config_with_properties(std::array{property});
    auto const to_merge = get_mouse_config_with_properties(std::array{property});

    target.merge(to_merge);

    mouse_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(TestInputConfiguration, TestMouseMergeOneProperty, ::testing::ValuesIn(all_mouse_props));

struct TestMouseMergeOnePropertyWithoutOverwrite :
    public TestInputConfiguration,
    testing::WithParamInterface<MouseProperty>
{
};

// Make sure tht merging does not alter other data members
TEST_P(TestMouseMergeOnePropertyWithoutOverwrite, mouse_merge_does_not_overwrite_values)
{
    auto const property = GetParam();

    auto const expected = get_mouse_config_with_properties(all_mouse_props);
    auto const modified = get_mouse_config_with_properties(std::array{property});
    auto target = get_mouse_config_with_all_properties_except(property);

    target.merge(modified);

    mouse_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration, TestMouseMergeOnePropertyWithoutOverwrite, ::testing::ValuesIn(all_mouse_props));

struct TestTouchpadInputConfiguration: public TestInputConfiguration
{
    Touchpad touch_config;
};

TEST_F(TestTouchpadInputConfiguration, touchpad_disable_while_typing_returns_expected_value)
{
    auto const value = false;
    touch_config.disable_while_typing(value);
    EXPECT_THAT(touch_config.disable_while_typing(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_disable_with_external_mouse_returns_expected_value)
{
    auto const value = true;
    touch_config.disable_with_external_mouse(value);

    EXPECT_THAT(touch_config.disable_with_external_mouse(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_acceleration_returns_expected_value)
{
    auto const value = mir_pointer_acceleration_adaptive;
    touch_config.acceleration(value);
    EXPECT_THAT(touch_config.acceleration(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_acceleration_bias_returns_expected_value)
{
    auto const value = -1;
    touch_config.acceleration_bias(value);
    EXPECT_THAT(touch_config.acceleration_bias(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_hscroll_speed_returns_expected_value)
{
    auto const value = -5;
    touch_config.hscroll_speed(value);
    EXPECT_THAT(touch_config.hscroll_speed(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_vscroll_speed_returns_expected_value)
{
    auto const value = 5;
    touch_config.vscroll_speed(value);
    EXPECT_THAT(touch_config.vscroll_speed(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_click_mode_returns_expected_value)
{
    auto const value = mir_touchpad_click_mode_area_to_click;
    touch_config.click_mode(value);
    EXPECT_THAT(touch_config.click_mode(), Eq(value));
}


TEST_F(TestTouchpadInputConfiguration, touchpad_scroll_mode_returns_expected_value)
{
    auto const value = mir_touchpad_scroll_mode_edge_scroll;
    touch_config.scroll_mode(value);
    EXPECT_THAT(touch_config.scroll_mode(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_tap_to_click_returns_expected_value)
{
    auto const value = false;
    touch_config.tap_to_click(value);
    EXPECT_THAT(touch_config.tap_to_click(), Eq(value));
}

TEST_F(TestTouchpadInputConfiguration, touchpad_middle_mouse_button_emulation_returns_expected_value)
{
    auto const value = true;
    touch_config.middle_mouse_button_emulation(value);
    EXPECT_THAT(touch_config.middle_mouse_button_emulation(), Eq(value));
}

struct TestTouchpadMergeOneProperty : public TestInputConfiguration, testing::WithParamInterface<TouchpadProperty>
{
    Touchpad target;
};

TEST_P(TestTouchpadMergeOneProperty, touchpad_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();

    auto const expected = get_touchpad_config_with_properties(all_touchpad_props);
    auto const modified = get_touchpad_config_with_properties(std::array{property});
    target = get_touchpad_config_with_all_properties_except(property);

    target.merge(modified);

    touchpad_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(TestInputConfiguration, TestTouchpadMergeOneProperty, ::testing::ValuesIn(all_touchpad_props));

struct TestTouchpadMergeOnePropertyWithoutOverwrite :
    public TestInputConfiguration,
    testing::WithParamInterface<TouchpadProperty>
{
};

TEST_P(TestTouchpadMergeOnePropertyWithoutOverwrite, touchapd_merge_does_not_overwrite_values)
{
    auto const property = GetParam();

    auto const expected = get_touchpad_config_with_properties(all_touchpad_props);
    auto const modified = get_touchpad_config_with_properties(std::array{property});
    auto target = get_touchpad_config_with_all_properties_except(property);

    target.merge(modified);

    touchpad_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration, TestTouchpadMergeOnePropertyWithoutOverwrite, ::testing::ValuesIn(all_touchpad_props));

struct TestKeyboardInputConfiguration: public TestInputConfiguration
{
    Keyboard keyboard_config;
};

TEST_F(TestKeyboardInputConfiguration, keyboard_repeat_rate_return_expected_values)
{
    Keyboard keyboard_config;
    auto const value = 731;
    keyboard_config.set_repeat_rate(value);
    EXPECT_THAT(keyboard_config.repeat_rate(), Eq(value));
}

TEST_F(TestKeyboardInputConfiguration, keyboard_repeat_delay_return_expected_values)
{
    Keyboard keyboard_config;
    auto const value = 2371;
    keyboard_config.set_repeat_delay(value);
    EXPECT_THAT(keyboard_config.repeat_delay(), Eq(value));
}

struct TestKeyboardMergeOneProperty : public TestInputConfiguration, testing::WithParamInterface<KeyboardProperty>
{
    Keyboard target;
};

TEST_P(TestKeyboardMergeOneProperty, keyboard_merge_from_partial_set_changes_only_set_values)
{
    auto const property = GetParam();

    auto const expected = get_keyboard_config_with_properties(std::array{property});
    auto const to_merge = get_keyboard_config_with_properties(std::array{property});

    target.merge(to_merge);

    keyboard_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(TestInputConfiguration, TestKeyboardMergeOneProperty, ::ValuesIn(all_keyboard_props));

struct TestKeyboardMergeOnePropertyWithoutOverwrite :
    public TestInputConfiguration,
    testing::WithParamInterface<KeyboardProperty>
{
};

TEST_P(TestKeyboardMergeOnePropertyWithoutOverwrite, keyboard_merge_does_not_overwrite_values)
{
    auto const property = GetParam();

    auto const expected = get_keyboard_config_with_properties(all_keyboard_props);
    auto const modified = get_keyboard_config_with_properties(std::array{property});
    auto target = get_keyboard_config_with_all_properties_except(property);

    target.merge(modified);

    keyboard_expect_equal(target, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TestInputConfiguration, TestKeyboardMergeOnePropertyWithoutOverwrite, ::testing::ValuesIn(all_keyboard_props));
