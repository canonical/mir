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

#include "mir/input/mousekeys_keymap.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"

#include "mir/shell/mousekeys_transformer.h"
#include "miral/mousekeys_config.h"
#include "miral/test_server.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class MockMouseKeysTransformer: public mir::shell::MouseKeysTransformer
{
public:
    MockMouseKeysTransformer() = default;
    MOCK_METHOD(void, keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, max_speed, (double x_axis, double y_axis), (override));
    MOCK_METHOD(bool, transform_input_event, (EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&), (override));
};

class MockAccessibilityManager: public mir::shell::AccessibilityManager
{
public:
    MockAccessibilityManager()
    {
        ON_CALL(*this, mousekeys()).WillByDefault(ReturnRef(mousekeys_transformer));
    }

    MOCK_METHOD(void, register_keyboard_helper, (std::shared_ptr<mir::shell::KeyboardHelper> const&), (override));
    MOCK_METHOD(std::optional<int>, repeat_rate, (), (const override));
    MOCK_METHOD(int, repeat_delay, (), (const override));
    MOCK_METHOD(void, repeat_rate_and_delay, (std::optional<int> new_rate, std::optional<int> new_delay), (override));
    MOCK_METHOD(void, notify_helpers, (), (const override));
    MOCK_METHOD(void, cursor_scale, (float), (override));
    MOCK_METHOD(void, mousekeys_enabled, (bool on), (override));
    MOCK_METHOD(mir::shell::MouseKeysTransformer&, mousekeys, (), (override));
    MOCK_METHOD(void, simulated_secondary_click_enabled, (bool enabled), (override));
    MOCK_METHOD(mir::shell::SimulatedSecondaryClickTransformer&, simulated_secondary_click, (), (override));
    MOCK_METHOD(void, hover_click_enabled, (bool), (override));
    MOCK_METHOD(mir::shell::HoverClickTransformer&, hover_click, (), (override));
    MOCK_METHOD(void, slow_keys_enabled, (bool enabled), (override));
    MOCK_METHOD(mir::shell::SlowKeysTransformer&, slow_keys, (), (override));
    MOCK_METHOD(void, sticky_keys_enabled, (bool), (override));
    MOCK_METHOD(mir::shell::StickyKeysTransformer&, sticky_keys, (), (override));

    testing::NiceMock<MockMouseKeysTransformer> mousekeys_transformer;
};

struct TestMouseKeysConfig : miral::TestServer
{
    TestMouseKeysConfig()
    {
        start_server_in_setup = false;
        add_server_init(
            [this](mir::Server& server)
            {
                server.override_the_accessibility_manager(
                    [&, this]
                    {
                        return accessibility_manager;
                    });
            });
    }

    miral::MouseKeysConfig config{miral::MouseKeysConfig::enabled()};
    std::shared_ptr<testing::NiceMock<MockAccessibilityManager>> accessibility_manager =
        std::make_shared<testing::NiceMock<MockAccessibilityManager>>();
};

TEST_F(TestMouseKeysConfig, enabling_mousekeys_from_miral_enables_it_in_the_accessibility_manager)
{
    // Once at startup, and twice when we call `set_keymap` below
    InSequence seq;
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));  // `true` passed to config where its defined
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));  // First call below
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(false)); // Second call below

    add_server_init(config);
    start_server();
    config.enable();
    config.disable();
}

MATCHER_P(KeymapEq, keymap, "")
{
    auto const keymap_to_unordered_map =
        [](mir::input::MouseKeysKeymap const& keymap)
    {
        std::unordered_map<mir::input::XkbSymkey, mir::input::MouseKeysKeymap::Action> key_action_map;
        keymap.for_each_key_action_pair([&](auto const key, auto const action)
                                     { key_action_map.insert({key, action}); });

        return key_action_map;
    };

    auto const arg_key_action_map = keymap_to_unordered_map(arg);
    auto const keymap_key_action_map = keymap_to_unordered_map(keymap);

    return arg_key_action_map == keymap_key_action_map;
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_keymap_sets_transformer_keymap)
{
    auto const test_keymap = mir::input::MouseKeysKeymap{};

    // We don't call `set_keymap` at startup
    EXPECT_CALL(accessibility_manager->mousekeys_transformer, keymap(KeymapEq(test_keymap)));

    add_server_init(config);
    start_server();
    config.set_keymap(test_keymap);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_acceleration_factors_sets_mousekeys_acceleration_factors)
{
    auto const [default_constant, default_linear, default_quadratic] = std::tuple{100.0, 100.0, 30.0};
    auto const [constant, linear, quadratic] = std::tuple{1.0, 1.0, 1.0};
    InSequence seq;
    // Defaults, called on server init
    EXPECT_CALL(
        accessibility_manager->mousekeys_transformer,
        acceleration_factors(default_constant, default_linear, default_quadratic));
    EXPECT_CALL(accessibility_manager->mousekeys_transformer, acceleration_factors(constant, linear, quadratic));

    add_server_init(config);
    start_server();
    config.set_acceleration_factors(constant, linear, quadratic);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_max_speed_sets_mousekeys_max_speed)
{
    auto const [default_max_x, default_max_y] = std::pair{400.0, 400.0};
    auto const [max_x, max_y] = std::pair{1.0, 1.0};

    InSequence seq;
    EXPECT_CALL(accessibility_manager->mousekeys_transformer, max_speed(default_max_x, default_max_y));
    EXPECT_CALL(accessibility_manager->mousekeys_transformer, max_speed(max_x, max_y));

    add_server_init(config);
    start_server();
    config.set_max_speed(max_x, max_y);
}

struct EnableMouseKeysOptionTest : public TestMouseKeysConfig, public WithParamInterface<bool>
{
};

TEST_P(EnableMouseKeysOptionTest, enable_mouse_keys_option_overrides_mousekeys_default_enabled_state)
{
    auto const enabled = GetParam();
    add_to_environment("MIR_SERVER_ENABLE_MOUSE_KEYS", enabled?  "1": "0");

    if(enabled)
        EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));
    else
        EXPECT_CALL(*accessibility_manager, mousekeys_enabled(_)).Times(0);


    add_server_init(config);
    start_server();
}

TEST_P(EnableMouseKeysOptionTest, mouse_keys_is_enabled_on_config_state_if_option_is_not_specified)
{
    auto const enabled = GetParam();

    if(enabled)
    {
        EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));
        add_server_init(miral::MouseKeysConfig::enabled());
    }
    else
    {
        EXPECT_CALL(*accessibility_manager, mousekeys_enabled(_)).Times(0);
        add_server_init(miral::MouseKeysConfig::disabled());
    }

    start_server();
}

INSTANTIATE_TEST_SUITE_P(TestMouseKeysConfig, EnableMouseKeysOptionTest, ::Values(false, true));
