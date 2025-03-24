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
#include "src/server/input/default_input_device_hub.h"
#include "src/server/shell/basic_accessibility_manager.h"
#include "src/server/shell/mouse_keys_transformer.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/glib_main_loop.h"
#include "mir/test/fake_shared.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/mousekeys_common.h"

#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_key_mapper.h"
#include "mir/test/doubles/mock_led_observer_registrar.h"
#include "mir/test/doubles/mock_server_status_listener.h"
#include "mir/test/doubles/advanceable_clock.h"

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

struct MockMouseKeysTransformer : public mir::input::MouseKeysTransformer
{
    MockMouseKeysTransformer() = default;

    MOCK_METHOD(void, set_keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, set_acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, set_max_speed, (double x_axis, double y_axis), (override));
    MOCK_METHOD(
        bool,
        transform_input_event,
        (mir::input::InputEventTransformer::EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&),
        (override));
};

struct TestBasicAccessibilityManager : Test
{
    TestBasicAccessibilityManager() :
        main_loop{std::make_shared<mir::GLibMainLoop>(mt::fake_shared(clock))},
        input_device_hub{std::make_shared<mir::input::DefaultInputDeviceHub>(
            mt::fake_shared(mock_seat),
            mt::fake_shared(multiplexer),
            mt::fake_shared(clock),
            mt::fake_shared(mock_key_mapper),
            mt::fake_shared(mock_server_status_listener),
            mt::fake_shared(led_observer_registrar))},
        input_event_transformer{std::make_shared<mir::input::InputEventTransformer>(input_device_hub, main_loop)},
        basic_accessibility_manager{
            main_loop,
            input_event_transformer,
            mt::fake_shared(clock),
            true,
            [&](mir::input::MouseKeysKeymap const& keymap,
                double acceleration_constant_factor,
                double acceleration_linear_factor,
                double acceleration_quadratic_factor,
                double max_speed_x,
                double max_speed_y)
            {
                return create_transformer(
                    keymap,
                    acceleration_constant_factor,
                    acceleration_linear_factor,
                    acceleration_quadratic_factor,
                    max_speed_x,
                    max_speed_y);
            }}
    {
        basic_accessibility_manager.register_keyboard_helper(mock_key_helper);
    }

    void SetUp() override
    {
        ON_CALL(*this, create_transformer(_, _, _, _, _, _)).WillByDefault(Return(mock_mousekeys_transformer));
    }

    MOCK_METHOD(
        std::shared_ptr<mir::input::MouseKeysTransformer>,
        create_transformer,
        (mir::input::MouseKeysKeymap const& keymap,
         double acceleration_constant_factor,
         double acceleration_linear_factor,
         double acceleration_quadratic_factor,
         double max_speed_x,
         double max_speed_y),
        ());

    mtd::AdvanceableClock clock;
    mir::dispatch::MultiplexingDispatchable multiplexer;
    NiceMock<mtd::MockLedObserverRegistrar> led_observer_registrar;
    NiceMock<mtd::MockInputSeat> mock_seat;
    NiceMock<mtd::MockKeyMapper> mock_key_mapper;
    NiceMock<mtd::MockServerStatusListener> mock_server_status_listener;
    std::shared_ptr<NiceMock<MockKeyboardHelper>> mock_key_helper{std::make_shared<NiceMock<MockKeyboardHelper>>()};
    std::shared_ptr<NiceMock<MockMouseKeysTransformer>> mock_mousekeys_transformer{
        std::make_shared<NiceMock<MockMouseKeysTransformer>>()};

    std::shared_ptr<mir::MainLoop> const main_loop;
    std::shared_ptr<mir::input::DefaultInputDeviceHub> const input_device_hub;
    std::shared_ptr<mir::input::InputEventTransformer> const input_event_transformer;

    mir::shell::BasicAccessibilityManager basic_accessibility_manager;
};

TEST_F(TestBasicAccessibilityManager, changing_repeat_rate_and_notifying_calls_repeat_info_changed)
{
    EXPECT_CALL(*mock_key_helper, repeat_info_changed({70}, _));
    basic_accessibility_manager.repeat_rate(70);
    basic_accessibility_manager.notify_helpers();
}

TEST_F(TestBasicAccessibilityManager, changing_repeat_delay_and_notifying_calls_repeat_info_changed)
{
    EXPECT_CALL(*mock_key_helper, repeat_info_changed(_, 1337));
    basic_accessibility_manager.repeat_delay(1337);
    basic_accessibility_manager.notify_helpers();
}

TEST_F(
    TestBasicAccessibilityManager,
    changing_repeat_rate_to_nullopt_and_notifying_calls_repeat_info_changed_with_zero_rate)
{
    EXPECT_CALL(*mock_key_helper, repeat_info_changed(std::optional(0), _));
    basic_accessibility_manager.repeat_rate({});
    basic_accessibility_manager.notify_helpers();
}

TEST_F(TestBasicAccessibilityManager, set_repeat_rate_value_is_the_same_as_the_get_value)
{
    auto const expected = 12;
    basic_accessibility_manager.repeat_rate(expected);

    ASSERT_EQ(basic_accessibility_manager.repeat_rate(), expected);
}

TEST_F(TestBasicAccessibilityManager, set_repeat_delay_value_is_the_same_as_the_get_value)
{
    auto const expected = 9001;
    basic_accessibility_manager.repeat_delay(expected);

    ASSERT_EQ(basic_accessibility_manager.repeat_delay(), expected);
}

TEST_F(TestBasicAccessibilityManager, set_mousekeys_enabled_creates_a_transformer)
{
    EXPECT_CALL(*this, create_transformer(_, _, _, _, _, _));

    basic_accessibility_manager.set_mousekeys_enabled(true);
}

TEST_F(TestBasicAccessibilityManager, multiple_set_mousekeys_enabled_create_only_one_transformer)
{
    EXPECT_CALL(*this, create_transformer(_, _, _, _, _, _));

    for(auto i = 0; i < 5; i++)
        basic_accessibility_manager.set_mousekeys_enabled(true);
}

TEST_F(TestBasicAccessibilityManager, set_mousekeys_enabled_followed_by_a_disable_followed_by_an_enable_creates_two_transformers)
{
    EXPECT_CALL(*this, create_transformer(_, _, _, _, _, _)).Times(2);

    basic_accessibility_manager.set_mousekeys_enabled(true);
    basic_accessibility_manager.set_mousekeys_enabled(false);
    basic_accessibility_manager.set_mousekeys_enabled(true);
}

namespace mir::input
{
    bool operator==(MouseKeysKeymap const& left, MouseKeysKeymap const& right)
    {
        std::vector<std::pair<XkbSymkey, MouseKeysKeymap::Action>> leftPairs, rightPairs;
        left.for_each_key_action_pair([&leftPairs](auto key, auto action){ leftPairs.emplace_back(key, action); });
        right.for_each_key_action_pair([&rightPairs](auto key, auto action){ rightPairs.emplace_back(key, action); });

        return leftPairs == rightPairs;
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

    EXPECT_CALL(*mock_mousekeys_transformer, set_keymap(keymap));

    basic_accessibility_manager.set_mousekeys_enabled(true);
    basic_accessibility_manager.set_mousekeys_keymap(keymap);
}

TEST_F(TestBasicAccessibilityManager, calling_set_acceleration_factors_calls_set_acceleration_factors_on_transformer)
{
    auto const constant = 12.9, linear = 3737.0, quadratic = 111.0;
    EXPECT_CALL(*mock_mousekeys_transformer, set_acceleration_factors(constant, linear, quadratic));

    basic_accessibility_manager.set_mousekeys_enabled(true);
    basic_accessibility_manager.set_acceleration_factors(constant, linear, quadratic);
}

TEST_F(TestBasicAccessibilityManager, calling_set_max_speed_calls_set_max_speed_on_transformer)
{
    auto const max_x = 100, max_y = 10710;
    EXPECT_CALL(*mock_mousekeys_transformer, set_max_speed(max_x, max_y));

    basic_accessibility_manager.set_mousekeys_enabled(true);
    basic_accessibility_manager.set_max_speed(max_x, max_y);
}
