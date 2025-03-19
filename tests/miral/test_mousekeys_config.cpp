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

#include "mir/input/mousekeys_common.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"

#include "miral/mousekeys_config.h"
#include "miral/test_server.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class MockAccessibilityManager: public mir::shell::AccessibilityManager
{
public:
    MOCK_METHOD(void, register_keyboard_helper, (std::shared_ptr<mir::shell::KeyboardHelper> const&), (override));
    MOCK_METHOD(std::optional<int>, repeat_rate, (), (const override));
    MOCK_METHOD(int, repeat_delay, (), (const override));
    MOCK_METHOD(void, repeat_rate, (int new_rate), (override));
    MOCK_METHOD(void, repeat_delay, (int new_rate), (override));
    MOCK_METHOD(void, notify_helpers, (), (const override));
    MOCK_METHOD(void, set_mousekeys_enabled, (bool on), (override));
    MOCK_METHOD(void, set_mousekeys_keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, set_acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, set_max_speed, (double x_axis, double y_axis), (override));
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

    miral::MouseKeysConfig config{true};
    std::shared_ptr<testing::NiceMock<MockAccessibilityManager>> accessibility_manager =
        std::make_shared<testing::NiceMock<MockAccessibilityManager>>();
};

TEST_F(TestMouseKeysConfig, mousekeys_config_set_mouse_keys_calls_accessibility_manager_set_mousekeys_enabled)
{
    // Once at startup, and twice when we call `set_keymap` below
    EXPECT_CALL(*accessibility_manager, set_mousekeys_enabled(_))
        .Times(3)
        .WillOnce(
            [](auto enabled)
            {
                // `true` passed to config where its defined
                ASSERT_TRUE(enabled);
            })
        .WillOnce(
            [](auto enabled)
            {
                // First call below
                ASSERT_TRUE(enabled);
            })
        .WillOnce(
            [](auto enabled)
            {
                // Second call below
                ASSERT_FALSE(enabled);
            });

    add_server_init(config);
    start_server();
    config.set_mousekeys_enabled(true);
    config.set_mousekeys_enabled(false);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_keymap_calls_accessibility_manager_set_mousekeys_keymap)
{
    // We don't call `set_keymap` at startup
    EXPECT_CALL(*accessibility_manager, set_mousekeys_keymap(_));

    add_server_init(config);
    start_server();
    config.set_keymap(mir::input::MouseKeysKeymap{});
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_acceleration_factors_calls_accessibility_manager_set_acceleration_factors)
{
    EXPECT_CALL(*accessibility_manager, set_acceleration_factors(_, _, _)).Times(2);

    add_server_init(config);
    start_server();
    config.set_acceleration_factors(1.0, 1.0, 1.0);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_max_speed_calls_accessibility_manager_set_max_speed)
{
    EXPECT_CALL(*accessibility_manager, set_max_speed(_, _)).Times(2);

    add_server_init(config);
    start_server();
    config.set_max_speed(1.0, 1.0);
}
