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

#include "mir/server.h"
#include "mir/shell/slow_keys_transformer.h"
#include "mir/shell/accessibility_manager.h"
#include "miral/slow_keys.h"
#include "miral/test_server.h"

#include "gmock/gmock.h"

using namespace testing;

class MockSlowKeysTransformer: public mir::shell::SlowKeysTransformer
{
public:
    MockSlowKeysTransformer()
    {
        ON_CALL(*this, on_key_down(_)).WillByDefault([](auto okd) { okd(0); });
        ON_CALL(*this, on_key_rejected(_)).WillByDefault([](auto okr) { okr(0); });
        ON_CALL(*this, on_key_accepted(_)).WillByDefault([](auto oka) { oka(0); });
    }

    MOCK_METHOD(void, on_key_down,(std::function<void(unsigned int)>&&), (override));
    MOCK_METHOD(void, on_key_rejected,(std::function<void(unsigned int)>&&), (override));
    MOCK_METHOD(void, on_key_accepted,(std::function<void(unsigned int)>&&), (override));
    MOCK_METHOD(void, delay,(std::chrono::milliseconds), (override));
    MOCK_METHOD(bool, transform_input_event, (EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&), (override));
};

// TODO: this is yanked from the mousekeys config test, we should pull these into mtd
// FIXME: Symbol name collision with the original from the mousekeys test
class FooBarBaz: public mir::shell::AccessibilityManager
{
public:
    FooBarBaz()
    {
        ON_CALL(*this, slow_keys()).WillByDefault(ReturnRef(slow_keys_transformer));
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

    testing::NiceMock<MockSlowKeysTransformer> slow_keys_transformer;
};


class TestSlowKeys : public miral::TestServer
{
public:
    TestSlowKeys()
    {
        // TODO: Dedup this, maybe create AccessibilityTestServer?
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

    miral::SlowKeys slow_keys{miral::SlowKeys::enabled()};
    std::shared_ptr<testing::NiceMock<FooBarBaz>> accessibility_manager =
        std::make_shared<testing::NiceMock<FooBarBaz>>();
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
