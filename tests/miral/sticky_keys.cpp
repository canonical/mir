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

#include "add_virtual_device.h"
#include "miral/sticky_keys.h"
#include "miral/test_server.h"

#include "mir/input/event_builder.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/server.h"

#include <linux/input-event-codes.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "miral/append_event_filter.h"

namespace mi = mir::input;
using namespace testing;

namespace
{
void click(mi::InputSink* sink, mi::EventBuilder* builder, unsigned int keysym, unsigned int scancode)
{
    sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, keysym, scancode));
    sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_up, keysym, scancode));
}
}

class TestStickyKeys : public miral::TestServer
{
public:
    TestStickyKeys()
    {
        add_server_init([this](mir::Server& server)
        {
            server.add_init_callback([&server, this]
            {
                input_device_registry = server.the_input_device_registry();
                input_device_hub = server.the_input_device_hub();
            });

            sticky_keys(server);
        });
    }

    miral::StickyKeys sticky_keys{miral::StickyKeys::enabled()};
    std::weak_ptr<mi::InputDeviceRegistry> input_device_registry;
    std::weak_ptr<mi::InputDeviceHub> input_device_hub;
};

TEST_F(TestStickyKeys, when_enabled_clicking_modifier_results_in_callback)
{
    int count = 0;
    sticky_keys.on_modifier_clicked([&](int32_t scan_code)
    {
        EXPECT_THAT(scan_code, Eq(KEY_LEFTSHIFT));
        count++;
    });

    auto const virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);
    virtual_keyboard->if_started_then(
        [&](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            click(sink, builder, XKB_KEY_Shift_L, KEY_LEFTSHIFT);
        });

    EXPECT_THAT(count, Eq(1));
}

TEST_F(TestStickyKeys, when_disabled_clicking_modifier_does_not_result_in_callback)
{
    int count = 0;
    sticky_keys.disable().on_modifier_clicked([&](int32_t)
    {
        count++;
    });

    auto const virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);
    virtual_keyboard->if_started_then(
        [&](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            click(sink, builder, XKB_KEY_Shift_L, KEY_LEFTSHIFT);
        });

    EXPECT_THAT(count, Eq(0));
}

TEST_F(TestStickyKeys, clicking_two_modifiers_simultaneously_results_in_no_modifiers_stuck)
{
    int count = 0;
    sticky_keys.should_disable_if_two_keys_are_pressed_together(true)
        .on_modifier_clicked([&](int32_t)
        {
            count++;
        });

    auto const virtual_keyboard = miral::test::add_test_device(
        input_device_registry.lock(), input_device_hub.lock(), mi::DeviceCapability::keyboard);
    virtual_keyboard->if_started_then(
        [&](mi::InputSink* sink, mi::EventBuilder* builder)
        {
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_Shift_L, KEY_LEFTSHIFT));
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_Shift_R, KEY_RIGHTSHIFT));

            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_up, XKB_KEY_Shift_L, KEY_LEFTSHIFT));
            sink->handle_input(builder->key_event(std::nullopt, mir_keyboard_action_up, XKB_KEY_Shift_R, KEY_RIGHTSHIFT));
        });

    EXPECT_THAT(count, Eq(0));
}
