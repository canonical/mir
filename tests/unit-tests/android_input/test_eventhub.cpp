/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <EventHub.h>

#include "src/server/report/null_report_factory.h"

#include "mir/udev/wrapper.h"
#include <umockdev.h>
#include "mir_test_framework/udev_environment.h"
#include "mir/test/wait_condition.h"
#include "mir/test/auto_unblock_thread.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/epoll.h>

#include <thread>
#include <cstring>

namespace mi = mir::input;

TEST(EventHub, does_not_block_on_get_events)
{
    mir_test_framework::UdevEnvironment empty_env;
    auto hub = android::sp<android::EventHub>{new android::EventHub{mir::report::null_input_report()}};
    auto get_event_terminates = std::make_shared<mir::test::WaitCondition>();

    mir::test::AutoJoinThread reader{
        [hub,get_event_terminates]
        {
            android::RawEvent buffer[10];
            memset(buffer, 0, sizeof(buffer));

            for (int i = 0; i != 10; ++i)
                hub->getEvents(buffer, 10);
            get_event_terminates->wake_up_everyone();
        }};

    get_event_terminates->wait_for_at_most_seconds(10);
    EXPECT_TRUE(get_event_terminates->woken());
}

TEST(EventHub, wakes_on_delay)
{
    mir_test_framework::UdevEnvironment empty_env;
    auto hub = android::sp<android::EventHub>{new android::EventHub{mir::report::null_input_report()}};
    auto hub_fd = hub->fd();
    hub->wakeIn(1);
    auto event_hub_triggered = std::make_shared<mir::test::WaitCondition>();

    mir::test::AutoJoinThread reader{
        [hub_fd,event_hub_triggered]
        {
            struct epoll_event pending_event;
            std::memset(&pending_event, 0, sizeof pending_event);
            
            auto ret = epoll_wait(hub_fd, &pending_event, 1, 3000);

            if (ret > 0 && pending_event.data.u32 == android::EventHub::EPOLL_ID_TIMER)
                event_hub_triggered->wake_up_everyone();
        }};

    event_hub_triggered->wait_for_at_most_seconds(4);
    EXPECT_TRUE(event_hub_triggered->woken());
}

class EventHubDeviceEnumerationTest : public ::testing::TestWithParam<std::string>
{
    // Don't actually need any shared state at the moment
};

TEST_P(EventHubDeviceEnumerationTest, ScansOnConstruction)
{
    mir_test_framework::UdevEnvironment env;
    env.add_standard_device(GetParam());

    auto hub = android::sp<android::EventHub>{new android::EventHub{mir::report::null_input_report()}};

    android::RawEvent buffer[10];
    memset(buffer, 0, sizeof(buffer));
    auto num_events = hub->getEvents(buffer, 10);

    EXPECT_EQ(static_cast<size_t>(3), num_events);
    EXPECT_EQ(android::EventHub::DEVICE_ADDED, buffer[0].type);
    EXPECT_EQ(android::VIRTUAL_KEYBOARD_ID, buffer[0].deviceId);
    EXPECT_EQ(android::EventHub::DEVICE_ADDED, buffer[1].type);
    EXPECT_EQ(1, buffer[1].deviceId);
    EXPECT_EQ(android::EventHub::FINISHED_DEVICE_SCAN, buffer[2].type);
}

TEST_P(EventHubDeviceEnumerationTest, GeneratesDeviceAddedOnHotplug)
{
    mir_test_framework::UdevEnvironment env;

    auto hub = android::sp<android::EventHub>{new android::EventHub{mir::report::null_input_report()}};

    android::RawEvent buffer[10];
    memset(buffer, 0, sizeof(buffer));
    auto num_events = hub->getEvents(buffer, 10);

    EXPECT_EQ(static_cast<size_t>(2), num_events);
    EXPECT_EQ(android::EventHub::DEVICE_ADDED, buffer[0].type);
    EXPECT_EQ(android::VIRTUAL_KEYBOARD_ID, buffer[0].deviceId);
    EXPECT_EQ(android::EventHub::FINISHED_DEVICE_SCAN, buffer[1].type);

    env.add_standard_device(GetParam());

    memset(buffer, 0, sizeof(buffer));
    num_events = hub->getEvents(buffer, 10);

    EXPECT_EQ(static_cast<size_t>(2), num_events);
    EXPECT_EQ(android::EventHub::DEVICE_ADDED, buffer[0].type);
    EXPECT_EQ(1, buffer[0].deviceId);
    EXPECT_EQ(android::EventHub::FINISHED_DEVICE_SCAN, buffer[1].type);
}

TEST_P(EventHubDeviceEnumerationTest, GeneratesDeviceRemovedOnHotunplug)
{
    mir_test_framework::UdevEnvironment env;
    env.add_standard_device(GetParam());

    auto hub = android::sp<android::EventHub>{new android::EventHub{mir::report::null_input_report()}};

    android::RawEvent buffer[10];
    // Flush out initial events.
    auto num_events = hub->getEvents(buffer, 10);

    mir::udev::Enumerator devices{std::make_shared<mir::udev::Context>()};
    devices.scan_devices();

    for (auto& device : devices)
    {
        /*
         * Remove just the device providing dev/input/event*
         * If we remove more, it's possible that we'll remove the parent of the
         * /dev/input device, and umockdev will not generate a remove event
         * in that case.
         */
        if (device.devnode() && (std::string(device.devnode()).find("input/event") != std::string::npos))
        {
            env.remove_device((std::string("/sys") + device.devpath()).c_str());
        }
    }

    memset(buffer, 0, sizeof(buffer));
    num_events = hub->getEvents(buffer, 10);

    EXPECT_EQ(static_cast<size_t>(2), num_events);
    EXPECT_EQ(android::EventHub::DEVICE_REMOVED, buffer[0].type);
    EXPECT_EQ(1, buffer[0].deviceId);
    EXPECT_EQ(android::EventHub::FINISHED_DEVICE_SCAN, buffer[1].type);

}

INSTANTIATE_TEST_CASE_P(VariousDevices,
                        EventHubDeviceEnumerationTest,
                        ::testing::Values(std::string("synaptics-touchpad"),
                                          std::string("usb-keyboard"),
                                          std::string("usb-mouse"),
                                          std::string("laptop-keyboard"),
                                          std::string("bluetooth-magic-trackpad")));
