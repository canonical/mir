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
    MOCK_METHOD(void, cursor_scale, (float), (override));
    MOCK_METHOD(void, mousekeys_enabled, (bool on), (override));
    MOCK_METHOD(void, mousekeys_keymap, (mir::input::MouseKeysKeymap const& new_keymap), (override));
    MOCK_METHOD(void, acceleration_factors, (double constant, double linear, double quadratic), (override));
    MOCK_METHOD(void, max_speed, (double x_axis, double y_axis), (override));
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

TEST_F(TestMouseKeysConfig, mousekeys_config_enabled_calls_accessibility_manager_set_mousekeys_enabled)
{
    // Once at startup, and twice when we call `set_keymap` below
    InSequence seq;
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));  // `true` passed to config where its defined
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(true));  // First call below
    EXPECT_CALL(*accessibility_manager, mousekeys_enabled(false)); // Second call below

    add_server_init(config);
    start_server();
    config.enabled(true);
    config.enabled(false);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_keymap_calls_accessibility_manager_set_mousekeys_keymap)
{
    // We don't call `set_keymap` at startup
    EXPECT_CALL(*accessibility_manager, mousekeys_keymap(_));

    add_server_init(config);
    start_server();
    config.set_keymap(mir::input::MouseKeysKeymap{});
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_acceleration_factors_calls_accessibility_manager_set_acceleration_factors)
{
    InSequence seq;
    EXPECT_CALL(*accessibility_manager, acceleration_factors(100.0, 100.0, 30.0)); // Defaults, called on server init
    EXPECT_CALL(*accessibility_manager, acceleration_factors(1.0, 1.0, 1.0));

    add_server_init(config);
    start_server();
    config.set_acceleration_factors(1.0, 1.0, 1.0);
}

TEST_F(TestMouseKeysConfig, mousekeys_config_set_max_speed_calls_accessibility_manager_set_max_speed)
{
    InSequence seq;
    EXPECT_CALL(*accessibility_manager, max_speed(400.0, 400.0));
    EXPECT_CALL(*accessibility_manager, max_speed(1.0, 1.0));

    add_server_init(config);
    start_server();
    config.set_max_speed(1.0, 1.0);
}
