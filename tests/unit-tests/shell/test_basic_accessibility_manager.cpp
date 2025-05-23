/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "include/server/mir/shell/keyboard_helper.h"
#include "mir/shell/hover_click_transformer.h"
#include "src/server/input/default_input_device_hub.h"
#include "src/server/shell/basic_accessibility_manager.h"
#include "src/server/shell/mouse_keys_transformer.h"
#include "src/server/shell/basic_simulated_secondary_click_transformer.h"

#include "mir/input/input_event_transformer.h"
#include "mir/input/mousekeys_keymap.h"
#include "mir/test/fake_shared.h"

#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/stub_cursor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>


using namespace testing;
namespace mt = mir::test;
namespace mtd = mt::doubles;

struct MockKeyboardHelper: public mir::shell::KeyboardHelper
{
    MockKeyboardHelper() = default;
    virtual ~MockKeyboardHelper() = default;
    MOCK_METHOD(void, repeat_info_changed, (std::optional<int> rate, int delay), (const override));
};

struct MockMouseKeysTransformer : public mir::shell::MouseKeysTransformer
{
    MockMouseKeysTransformer() = default;

    MOCK_METHOD(void, keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, max_speed, (double x_axis, double y_axis), (override));
    MOCK_METHOD(
        bool,
        transform_input_event,
        (mir::input::InputEventTransformer::EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&),
        (override));
};

struct MockSimulatedSecondaryClickTransformer : public mir::shell::SimulatedSecondaryClickTransformer
{
    MockSimulatedSecondaryClickTransformer() = default;

    MOCK_METHOD(
        bool,
        transform_input_event,
        (mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
         mir::input::EventBuilder*,
         MirEvent const&),
        (override));

    MOCK_METHOD(void, hold_duration, (std::chrono::milliseconds delay), (override));
    MOCK_METHOD(void, displacement_threshold, (float), (override));
    MOCK_METHOD(void, on_hold_start, (std::function<void()>&& on_hold_start), (override));
    MOCK_METHOD(void, on_hold_cancel, (std::function<void()>&& on_hold_cancel), (override));
    MOCK_METHOD(void, on_secondary_click, (std::function<void()>&& on_secondary_click), (override));
};

struct MockInputEventTransformer: public mir::input::InputEventTransformer
{
    MockInputEventTransformer(std::shared_ptr<Seat> const& seat, std::shared_ptr<mir::time::Clock> const& clock)
        : mir::input::InputEventTransformer(seat, clock)
    {
    }

    MOCK_METHOD(void, append, (std::weak_ptr<Transformer> const&), (override));
    MOCK_METHOD(void, remove, (std::shared_ptr<Transformer> const&), (override));
};

struct MockHoverClickTransformer : public mir::shell::HoverClickTransformer
{
    MockHoverClickTransformer() = default;
    MOCK_METHOD(void, hover_duration,(std::chrono::milliseconds delay), (override));
    MOCK_METHOD(void, cancel_displacement_threshold,(float displacement), (override));
    MOCK_METHOD(void, enabled,(), (override));
    MOCK_METHOD(void, disabled,(), (override));
    MOCK_METHOD(void, on_enabled,(std::function<void()>&& on_enabled), (override));
    MOCK_METHOD(void, on_disabled,(std::function<void()>&& on_disabled), (override));
    MOCK_METHOD(void, on_hover_start,(std::function<void()>&& on_hover_start), (override));
    MOCK_METHOD(void, on_hover_cancel,(std::function<void()>&& on_hover_cancelled), (override));
    MOCK_METHOD(void, on_click_dispatched,(std::function<void()>&& on_click_dispatched), (override));
    MOCK_METHOD(
        bool,
        transform_input_event,
        (mir::input::InputEventTransformer::EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&),
        (override));
};

struct TestBasicAccessibilityManager : Test
{
    TestBasicAccessibilityManager() :
        basic_accessibility_manager{
            mt::fake_shared(input_event_transformer),
            true,
            std::make_shared<mir::test::doubles::StubCursor>(),
            mock_mousekeys_transformer,
            mock_simulated_secondary_click_transformer,
            std::make_shared<MockHoverClickTransformer>()}
    {
        basic_accessibility_manager.register_keyboard_helper(mock_key_helper);
    }

    mtd::AdvanceableClock clock;
    NiceMock<mtd::MockInputSeat> mock_seat;

    std::shared_ptr<NiceMock<MockKeyboardHelper>> mock_key_helper{std::make_shared<NiceMock<MockKeyboardHelper>>()};
    std::shared_ptr<NiceMock<MockMouseKeysTransformer>> mock_mousekeys_transformer{
        std::make_shared<NiceMock<MockMouseKeysTransformer>>()};
    std::shared_ptr<NiceMock<MockSimulatedSecondaryClickTransformer>> mock_simulated_secondary_click_transformer{
        std::make_shared<NiceMock<MockSimulatedSecondaryClickTransformer>>()};

    NiceMock<MockInputEventTransformer> input_event_transformer{mt::fake_shared(mock_seat), mt::fake_shared(clock)};

    mir::shell::BasicAccessibilityManager basic_accessibility_manager;
};

TEST_F(TestBasicAccessibilityManager, changing_repeat_rate_and_notifying_calls_repeat_info_changed)
{
    EXPECT_CALL(*mock_key_helper, repeat_info_changed({70}, _));
    basic_accessibility_manager.repeat_rate_and_delay(70, {});
}

TEST_F(TestBasicAccessibilityManager, changing_repeat_delay_and_notifying_calls_repeat_info_changed)
{
    EXPECT_CALL(*mock_key_helper, repeat_info_changed(_, 1337));
    basic_accessibility_manager.repeat_rate_and_delay({}, 1337);
}

struct RepeatRateAndDelayParams {
    std::optional<int> input_rate;
    std::optional<int> input_delay;

    std::optional<std::pair<int, int>> expected_rate_and_delay;
};

struct TestRepeatRateAndDelaySetter :
    public TestBasicAccessibilityManager,
    public WithParamInterface<RepeatRateAndDelayParams>
{
};

TEST_P(
    TestRepeatRateAndDelaySetter,
    nullopt_repeat_rate_and_delay_dont_notify_helpers)
{
    auto const [input_rate, input_delay, expected] = GetParam();

    auto const number_of_calls = expected.has_value() ? 1 : 0;
    constexpr auto dont_care = std::pair<int, int>(0, 0);
    EXPECT_CALL(
        *mock_key_helper,
        repeat_info_changed({expected.value_or(dont_care).first}, expected.value_or(dont_care).second))
        .Times(number_of_calls);

    basic_accessibility_manager.repeat_rate_and_delay(input_rate, input_delay);
}

INSTANTIATE_TEST_SUITE_P(
    TestBasicAccessibilityManager,
    TestRepeatRateAndDelaySetter,
    ::Values(
        // Setting the rate only uses the new rate and the default delay
        RepeatRateAndDelayParams{{53}, {}, {{53, 600}}},
        // Setting the delay only uses the new delay and the default rate
        RepeatRateAndDelayParams{{}, {2353}, {{25, 2353}}},
        RepeatRateAndDelayParams{{43}, {2107}, {{43, 2107}}},

        // Setting neither should not call `repeat_info_changed`
        // don't care about the last two parameters since the method is not called anyway
        RepeatRateAndDelayParams{{}, {}, {}},
        // Setting both to the default values shouldn't also call it ^
        RepeatRateAndDelayParams{{25}, {600}, {}},
        // Setting one or the other to default values shouldn't call it ^^
        RepeatRateAndDelayParams{{}, {600}, {}},
        RepeatRateAndDelayParams{{25}, {}, {}}));

TEST_F(TestBasicAccessibilityManager, set_repeat_rate_value_is_the_same_as_the_get_value)
{
    auto const expected = 12;
    basic_accessibility_manager.repeat_rate_and_delay(expected, {});

    ASSERT_EQ(basic_accessibility_manager.repeat_rate(), expected);
}

TEST_F(TestBasicAccessibilityManager, set_repeat_delay_value_is_the_same_as_the_get_value)
{
    auto const expected = 9001;
    basic_accessibility_manager.repeat_rate_and_delay({}, expected);

    ASSERT_EQ(basic_accessibility_manager.repeat_delay(), expected);
}

namespace
{
MATCHER_P(KeymapMatches, expected, "")
{
    std::vector<std::pair<mir::input::XkbSymkey, mir::input::MouseKeysKeymap::Action>> expectedPairs, actualPairs;

    expected.for_each_key_action_pair(
        [&expectedPairs](auto key, auto action)
        {
            expectedPairs.emplace_back(key, action);
        });

    arg.for_each_key_action_pair(
        [&actualPairs](auto key, auto action)
        {
            actualPairs.emplace_back(key, action);
        });

    return expectedPairs == actualPairs;
}
}

TEST_F(TestBasicAccessibilityManager, calling_set_mousekeys_keymap_calls_set_keymap_on_transformer)
{
    using enum mir::input::MouseKeysKeymap::Action;
    auto const keymap = mir::input::MouseKeysKeymap{{
        {XKB_KEY_w, move_up},
        {XKB_KEY_s, move_down},
        {XKB_KEY_a, move_left},
        {XKB_KEY_d, move_right},
    }};

    EXPECT_CALL(*mock_mousekeys_transformer, keymap(KeymapMatches(keymap)));

    basic_accessibility_manager.mousekeys_enabled(true);
    basic_accessibility_manager.mousekeys_keymap(keymap);
}

TEST_F(TestBasicAccessibilityManager, calling_set_acceleration_factors_calls_set_acceleration_factors_on_transformer)
{
    auto const constant = 12.9, linear = 3737.0, quadratic = 111.0;
    EXPECT_CALL(*mock_mousekeys_transformer, acceleration_factors(constant, linear, quadratic));

    basic_accessibility_manager.mousekeys_enabled(true);
    basic_accessibility_manager.acceleration_factors(constant, linear, quadratic);
}

TEST_F(TestBasicAccessibilityManager, calling_set_max_speed_calls_set_max_speed_on_transformer)
{
    auto const max_x = 100, max_y = 10710;
    EXPECT_CALL(*mock_mousekeys_transformer, max_speed(max_x, max_y));

    basic_accessibility_manager.mousekeys_enabled(true);
    basic_accessibility_manager.max_speed(max_x, max_y);
}

TEST_F(TestBasicAccessibilityManager, calling_simulated_secondary_click_hold_duration_calls_the_corresponding_method_on_transformer)
{
    auto const expected_duration = std::chrono::milliseconds{333};
    EXPECT_CALL(*mock_simulated_secondary_click_transformer, hold_duration(expected_duration));

    basic_accessibility_manager.simulated_secondary_click_enabled(true);
    basic_accessibility_manager.simulated_secondary_click().hold_duration(expected_duration);
}

TEST_F(TestBasicAccessibilityManager, setting_on_hold_start_sets_it_on_transformer)
{
    auto const expected_on_hold_start = []
    {
    };

    EXPECT_CALL(*mock_simulated_secondary_click_transformer, on_hold_start(_));

    basic_accessibility_manager.simulated_secondary_click().on_hold_start(expected_on_hold_start);
}

TEST_F(TestBasicAccessibilityManager, setting_on_hold_cancel_sets_it_on_transformer)
{
    auto const expected_on_hold_cancel = []
    {
    };

    EXPECT_CALL(*mock_simulated_secondary_click_transformer, on_hold_cancel(_));

    basic_accessibility_manager.simulated_secondary_click().on_hold_cancel(expected_on_hold_cancel);
}

TEST_F(TestBasicAccessibilityManager, setting_on_secondary_click_sets_it_on_transformer)
{
    auto const expected_on_secondary_click = []
    {
    };

    EXPECT_CALL(*mock_simulated_secondary_click_transformer, on_secondary_click(_));

    basic_accessibility_manager.simulated_secondary_click().on_secondary_click(expected_on_secondary_click);
}


TEST_F(TestBasicAccessibilityManager, enabling_simulated_secondary_click_twice_calls_enabled_once)
{
    EXPECT_CALL(input_event_transformer, append(_)).Times(1);

    basic_accessibility_manager.simulated_secondary_click_enabled(true);
    basic_accessibility_manager.simulated_secondary_click_enabled(true);
}

TEST_F(
    TestBasicAccessibilityManager, enabling_simulated_secondary_click_then_disabling_then_enabling_calls_enabled_twice)
{
    EXPECT_CALL(input_event_transformer, append(_)).Times(2);

    basic_accessibility_manager.simulated_secondary_click_enabled(true);
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
    basic_accessibility_manager.simulated_secondary_click_enabled(true);
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
}


TEST_F(TestBasicAccessibilityManager, disabling_simulated_secondary_click_twice_calls_disabled_once)
{
    EXPECT_CALL(input_event_transformer, append(_)).Times(1);

    basic_accessibility_manager.simulated_secondary_click_enabled(true); // Have to enable to be able to disable
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
}

TEST_F(
    TestBasicAccessibilityManager, disabling_simulated_secondary_click_then_enabling_then_disabling_calls_disabled_twice)
{
    EXPECT_CALL(input_event_transformer, append(_)).Times(2);

    basic_accessibility_manager.simulated_secondary_click_enabled(true); // Have to enable to be able to disable
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
    basic_accessibility_manager.simulated_secondary_click_enabled(true);
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
}

TEST_F(
    TestBasicAccessibilityManager, disabling_simulated_secondary_click_before_enabling_it_doesnt_call_disabled)
{
    EXPECT_CALL(input_event_transformer, remove(_)).Times(0);
    basic_accessibility_manager.simulated_secondary_click_enabled(false);
}
