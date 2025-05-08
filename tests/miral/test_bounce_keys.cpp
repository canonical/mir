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

#include "add_virtual_device.h"

#include "miral/bounce_keys.h"
#include "miral/test_server.h"

#include "mir/input/event_builder.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/server.h"
#include "mir/test/signal.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
static constexpr auto test_bounce_keys_delay = 20ms;
}

struct TestBounceKeys : miral::TestServer
{
    TestBounceKeys()
    {
        add_server_init(
            [this](mir::Server& server)
            {
                server.add_init_callback(
                    [&server, this]
                    {
                        composite_event_filter = server.the_composite_event_filter();
                        input_device_registry = server.the_input_device_registry();
                        input_device_hub = server.the_input_device_hub();
                    });

                bounce_keys(server);
            });

        bounce_keys.delay(test_bounce_keys_delay)
            .on_press_rejected([this](auto) { press_rejected.raise(); });
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
    }

    miral::BounceKeys bounce_keys{true};
    mir::test::Signal press_rejected;

    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;
    std::weak_ptr<mir::input::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub;
};

struct TestDifferentDelays: public TestBounceKeys, public WithParamInterface<std::chrono::milliseconds>
{
};

TEST_P(TestDifferentDelays, subsequent_keys_rejected_if_in_within_delay)
{
    auto const press_delay = GetParam();
    auto virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mir::input::DeviceCapability::keyboard);

    virtual_keyboard->if_started_then(
        [this, press_delay](mir::input::InputSink* sink, mir::input::EventBuilder* builder)
        {
            auto rejection_counter = 0;
            bounce_keys.on_press_rejected(
                [&rejection_counter, this](auto)
                {
                    rejection_counter++;
                    press_rejected.raise();
                });

            auto const press_d = [builder, sink]()
            {
                sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_d, 32));
                sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_up, XKB_KEY_d, 32));
            };

            // Initial press, should pass
            press_d();

            // Depending on the test parameter, subsequent presses should be all rejected or not
            auto const num_presses = 10;
            for (auto i = 0; i < num_presses; i++)
            {
                std::this_thread::sleep_for(press_delay);
                press_d();
            }

            auto const should_reject_all = press_delay < test_bounce_keys_delay;
            EXPECT_THAT(rejection_counter, Eq(should_reject_all? num_presses : 0));
        });
}

INSTANTIATE_TEST_SUITE_P(TestBounceKeys, TestDifferentDelays, Values(test_bounce_keys_delay - 5ms, test_bounce_keys_delay + 5ms));
