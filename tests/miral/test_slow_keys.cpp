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

#include "miral/slow_keys.h"
#include "miral/accessibility_test_server.h"

#include <gmock/gmock.h>

using namespace testing;

class TestSlowKeys : public miral::TestAccessibilityManager
{
public:
    miral::SlowKeys slow_keys{miral::SlowKeys::enabled()};
};

TEST_F(TestSlowKeys, enabling_slow_keys_from_miral_enables_it_in_the_accessibility_manager)
{

    InSequence seq;
    // Called on server start since we enabled slow keys above
    EXPECT_CALL(*accessibility_manager, slow_keys_enabled(true));
    
    // Each one corresponds to a call below
    EXPECT_CALL(*accessibility_manager, slow_keys_enabled(true));
    EXPECT_CALL(*accessibility_manager, slow_keys_enabled(false));

    add_server_init(slow_keys);
    start_server();
    
    slow_keys.enable();
    slow_keys.disable();
}

TEST_F(TestSlowKeys, slow_keys_setting_on_key_down_sets_transformer_on_key_down)
{
    auto calls = 0;
    auto on_key_down = [&calls](auto) mutable
    {
        calls += 1;
    };

    InSequence seq;
    // First call is the default empty callback, and the second one is the callback pass.
    // Unfortunately we can't check the equality of the callbacks.
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_down(_)).Times(2);

    add_server_init(slow_keys);

    // Gets stored in `SlowKeys` and is set when the server starts
    slow_keys.on_key_down(on_key_down);
    EXPECT_THAT(calls, Eq(0));

    start_server();
    EXPECT_THAT(calls, Eq(1));

    slow_keys.on_key_down(on_key_down);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, slow_keys_setting_on_key_rejected_sets_transformer_on_key_rejected)
{
    auto calls = 0;
    auto on_key_rejected = [&calls](auto) mutable
    {
        calls += 1;
    };

    InSequence seq;
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_rejected(_)).Times(2);

    add_server_init(slow_keys);

    slow_keys.on_key_rejected(on_key_rejected);
    EXPECT_THAT(calls, Eq(0));

    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    slow_keys.on_key_rejected(on_key_rejected);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, slow_keys_setting_on_key_accepted_sets_transformer_on_key_accepted)
{
    auto calls = 0;
    auto on_key_accepted = [&calls](auto) mutable
    {
        calls += 1;
    };
    
    InSequence seq;
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_accepted(_)).Times(2);

    add_server_init(slow_keys);
    
    slow_keys.on_key_accepted(on_key_accepted);
    EXPECT_THAT(calls, Eq(0));

    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    slow_keys.on_key_accepted(on_key_accepted);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, slow_keys_setting_delay_sets_transformer_delay)
{
    auto delay1 = std::chrono::milliseconds{1337};
    auto delay2 = std::chrono::milliseconds{0xF00F};

    InSequence seq;
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, delay(delay1));
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, delay(delay2));

    add_server_init(slow_keys);

    slow_keys.hold_delay(delay1);
    start_server();
    slow_keys.hold_delay(delay2);
}
