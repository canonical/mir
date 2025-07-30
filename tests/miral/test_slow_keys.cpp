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
    TestSlowKeys()
    {
        ON_CALL(accessibility_manager->slow_keys_transformer, on_key_down(_))
            .WillByDefault([](auto okd_cb) { okd_cb(0); });
        ON_CALL(accessibility_manager->slow_keys_transformer, on_key_rejected(_))
            .WillByDefault([](auto okr_cb) { okr_cb(0); });
        ON_CALL(accessibility_manager->slow_keys_transformer, on_key_accepted(_))
            .WillByDefault([](auto oka_cb) { oka_cb(0); });
    }

    miral::SlowKeys slow_keys{miral::SlowKeys::enabled()};
};

TEST_F(TestSlowKeys, enabling_from_miral_enables_it_in_the_accessibility_manager)
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

TEST_F(TestSlowKeys, setting_on_key_down_sets_transformer_on_key_down)
{
    auto calls = 0;
    auto okd = [&calls](auto){ calls += 1; };
    add_server_init(slow_keys);

    InSequence seq;

    // The server hasn't started, so the transformer is unavailable. The
    // callback is stored in `miral::SlowKeys` until the server starts.
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_down(_)).Times(0);
    slow_keys.on_key_down(okd);
    EXPECT_THAT(calls, Eq(0));

    // When the server starts, all data is transferred from the miral side to
    // the transformer. So `on_key_down` should get called
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_down(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    // After the server starts, all modifications from miral get relayed
    // directly to the transformer. So `on_key_down` will get called once
    // again.
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_down(_)).Times(1);
    slow_keys.on_key_down(okd);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, setting_on_key_rejected_sets_transformer_on_key_rejected)
{
    auto calls = 0;
    auto okr = [&calls](auto){ calls += 1; };
    add_server_init(slow_keys);

    InSequence seq;

    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_rejected(_)).Times(0);
    slow_keys.on_key_rejected(std::move(okr));
    EXPECT_THAT(calls, Eq(0));

    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_rejected(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_rejected(_)).Times(1);
    slow_keys.on_key_rejected(std::move(okr));
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, setting_on_key_accepted_sets_transformer_on_key_accepted)
{
    auto calls = 0;
    auto oka = [&calls](auto){ calls += 1; };
    add_server_init(slow_keys);

    InSequence seq;

    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_accepted(_)).Times(0);
    slow_keys.on_key_accepted(std::move(oka));
    EXPECT_THAT(calls, Eq(0));

    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_accepted(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    EXPECT_CALL(accessibility_manager->slow_keys_transformer, on_key_accepted(_)).Times(1);
    slow_keys.on_key_accepted(std::move(oka));
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestSlowKeys, setting_delay_sets_transformer_delay)
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
