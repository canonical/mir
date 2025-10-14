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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace mi = mir::input;

namespace
{
static constexpr auto test_bounce_keys_delay = 40ms;

void press(mi::InputSink* sink, mi::EventBuilder* builder, unsigned int keysym, unsigned int scancode)
{
    sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, keysym, scancode));
    sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_up, keysym, scancode));
}
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

        bounce_keys.delay(test_bounce_keys_delay);
    }

    miral::BounceKeys bounce_keys{miral::BounceKeys::enabled()};

    std::weak_ptr<mi::CompositeEventFilter> composite_event_filter;
    std::weak_ptr<mi::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mi::InputDeviceHub> input_device_hub;
};

struct TestDifferentDelays: public TestBounceKeys, public WithParamInterface<std::chrono::milliseconds>
{
};

TEST_P(TestDifferentDelays, subsequent_keys_rejected_if_in_within_delay)
{
    auto const press_delay = GetParam();

    auto virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);

    virtual_keyboard->if_started_then(
        [this, press_delay](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            std::atomic rejection_counter = 0;
            bounce_keys.on_press_rejected([&rejection_counter](auto) { rejection_counter++; });

            // Initial press, should pass
            press(sink, builder, XKB_KEY_d, 32);

            // Depending on the test parameter, subsequent presses should be all rejected or not
            auto const num_presses = 10;
            for (auto i = 0; i < num_presses; i++)
            {
                std::this_thread::sleep_for(press_delay);
                press(sink, builder, XKB_KEY_d, 32);
            }

            auto const should_reject_all = press_delay < test_bounce_keys_delay;
            EXPECT_THAT(rejection_counter, Eq(should_reject_all? num_presses : 0));
        });
}

INSTANTIATE_TEST_SUITE_P(TestBounceKeys, TestDifferentDelays, Values(test_bounce_keys_delay - 5ms, test_bounce_keys_delay + 5ms));

TEST_F(TestBounceKeys, different_keys_are_not_rejected)
{
    auto virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);

    virtual_keyboard->if_started_then(
        [this](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            std::atomic rejection_counter = 0;
            bounce_keys.on_press_rejected([&rejection_counter](auto) { rejection_counter++; });

            // Initial press, should pass
            press(sink, builder, XKB_KEY_d, 32);

            auto const num_presses = 20;
            for (auto i = 0; i < num_presses; i++)
            {
                // Would be rejected if the same key was being pressed
                std::this_thread::sleep_for(test_bounce_keys_delay - 1ms);

                if (i % 2 == 0)
                    press(sink, builder, XKB_KEY_f, 33);
                else
                    press(sink, builder, XKB_KEY_d, 32);
            }

            EXPECT_THAT(rejection_counter, Eq(0));
        });
}

TEST_F(TestBounceKeys, irregular_press_periods_are_properly_rejected)
{
    auto virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);

    auto const tap_periods = std::array{
        0.75 * test_bounce_keys_delay,
        1.1 * test_bounce_keys_delay,
        1.35 * test_bounce_keys_delay}; // reject, pass, pass

    virtual_keyboard->if_started_then(
        [this, &tap_periods](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            std::atomic rejection_counter = 0;
            bounce_keys.on_press_rejected([&rejection_counter](auto) { rejection_counter++; });

            auto const num_presses = 10;
            for (auto i = 0; i < num_presses; i++)
            {
                press(sink, builder, XKB_KEY_d, 32);
                std::this_thread::sleep_for(tap_periods[i % tap_periods.size()]);
            }

            EXPECT_THAT(rejection_counter, Eq(num_presses / tap_periods.size()));
        });
}
