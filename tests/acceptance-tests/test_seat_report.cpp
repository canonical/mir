/*
 * Copyright © 2017 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/input/input_device_info.h"
#include "mir/input/keymap.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/input_device.h"
#include "mir/input/device.h"
#include "mir/input/touchscreen_settings.h"

#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir/test/signal.h"
#include "mir/test/event_matchers.h"
#include "mir/test/event_factory.h"

#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/seat_observer.h"
#include "mir/observer_registrar.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mis = mir::input::synthesis;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

using namespace std::chrono_literals;
using namespace testing;

using TestSeatReport = mtf::HeadlessInProcessServer;

namespace
{
class NullSeatListener : public mi::SeatObserver
{
public:
    void seat_add_device(uint64_t /*id*/) override {}
    void seat_remove_device(uint64_t /*id*/) override {}
    void seat_dispatch_event(std::shared_ptr<MirEvent const> const& /*event*/) override {}

    void seat_set_key_state(
        uint64_t /*id*/,
        std::vector<uint32_t> const& /*scan_codes*/) override
    {
    }

    void seat_set_pointer_state(uint64_t /*id*/, unsigned /*buttons*/) override {}
    void seat_set_cursor_position(float /*cursor_x*/, float /*cursor_y*/) override {}
    void seat_set_confinement_region_called(geom::Rectangles const& /*regions*/) override {}
    void seat_reset_confinement_regions() override {}
};
}

TEST_F(TestSeatReport, add_device_received)
{
    class DeviceAddListener : public NullSeatListener
    {
    public:
        DeviceAddListener(size_t expected_devices)
            : expected_devices{expected_devices}
        {
        }

        void seat_add_device(uint64_t id) override
        {
            seen_ids.push_back(id);
            if (seen_ids.size() == expected_devices)
            {
                seen_expected_devices.raise();
            }
        }

        std::vector<uint64_t> wait_for_devices()
        {
            seen_expected_devices.wait_for(30s);
            return std::move(seen_ids);
        }
    private:
        size_t const expected_devices;
        std::vector<uint64_t> seen_ids;
        mt::Signal seen_expected_devices;
    };

    auto listener = std::make_shared<DeviceAddListener>(3);
    server.the_seat_observer_registrar()->register_interest(listener);

    auto constexpr mouse_name = "mouse";
    auto constexpr mouse_uid = "some-sort-of-uid";
    auto fake_pointer = mtf::add_fake_input_device(
        mi::InputDeviceInfo{mouse_name, mouse_uid, mi::DeviceCapability::pointer});

    auto constexpr touch_name = "touchscreen";
    auto constexpr touch_uid = "Bottle Rocket";
    auto fake_touch = mtf::add_fake_input_device(
        mi::InputDeviceInfo{touch_name, touch_uid, mi::DeviceCapability::touchscreen});

    auto constexpr keyboard_name = "keybored";
    auto constexpr keyboard_uid = "Panther Dash";
    auto fake_keyboard = mtf::add_fake_input_device(
        mi::InputDeviceInfo{
            keyboard_name,
            keyboard_uid,
            mi::DeviceCapability::alpha_numeric | mi::DeviceCapability::keyboard});

    auto device_ids_seen = listener->wait_for_devices();
    std::vector<uint64_t> devices;

    server.the_input_device_hub()->for_each_input_device(
        [&devices](mi::Device const& device)
        {
            devices.push_back(device.id());
        });

    EXPECT_THAT(device_ids_seen, ContainerEq(devices));
}

TEST_F(TestSeatReport, remove_device_received)
{
    class DeviceRemoveListener : public NullSeatListener
    {
    public:
        DeviceRemoveListener(size_t expected_devices)
            : expected_devices{expected_devices}
        {
        }

        void seat_add_device(uint64_t id) override
        {
            seen_ids.push_back(id);
        }

        void seat_remove_device(uint64_t id) override
        {
            removed_ids.push_back(id);
            if (removed_ids.size() == expected_devices)
                seen_expected_devices.raise();
        }

        bool wait_for_all_device_removals()
        {
            bool const waited = seen_expected_devices.wait_for(30s);

            EXPECT_THAT(removed_ids, UnorderedElementsAreArray(seen_ids));
            return waited;
        }
    private:
        size_t const expected_devices;
        std::vector<uint64_t> seen_ids;
        std::vector<uint64_t> removed_ids;
        mt::Signal seen_expected_devices;
    };

    auto listener = std::make_shared<DeviceRemoveListener>(3);
    server.the_seat_observer_registrar()->register_interest(listener);

    auto fake_pointer = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"name", "uid", mi::DeviceCapability::pointer});
    auto fake_touch = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"name", "uid", mi::DeviceCapability::touchscreen});
    auto fake_keyboard = mtf::add_fake_input_device(
        mi::InputDeviceInfo{
            "name",
            "uid",
            mi::DeviceCapability::alpha_numeric | mi::DeviceCapability::keyboard});

    fake_keyboard->emit_device_removal();
    fake_pointer->emit_device_removal();
    fake_touch->emit_device_removal();

    EXPECT_TRUE(listener->wait_for_all_device_removals());
}

namespace
{
class DeviceAwaitingObserver : public NullSeatListener
{
public:
    DeviceAwaitingObserver(size_t expected_devices)
        : expected_devices{expected_devices}
    {
    }

    void seat_add_device(uint64_t /*id*/) override
    {
        devices_seen++;
        if (devices_seen == expected_devices)
        {
            seen_expected_devices.raise();
        }
    }

    template<typename Period, typename Ratio>
    bool wait_for_expected_devices(std::chrono::duration<Period, Ratio> timeout)
    {
        return seen_expected_devices.wait_for(timeout);
    };
private:
    size_t const expected_devices;
    size_t devices_seen{0};
    mt::Signal seen_expected_devices;
};
}

TEST_F(TestSeatReport, dispatch_event_received)
{
    int const event_x = 234;
    int const event_y = 2097;
    class DispatchEventListener : public DeviceAwaitingObserver
    {
    public:
        DispatchEventListener(size_t expected_devices)
            : DeviceAwaitingObserver(expected_devices)
        {
        }

        void seat_dispatch_event(std::shared_ptr<MirEvent const> const& event) override
        {
            if (mir_event_get_type(event.get()) == mir_event_type_input)
            {
                auto iev = mir_event_get_input_event(event.get());
                if (mir_input_event_get_type(iev) == mir_input_event_type_pointer)
                {
                    auto pointer = mir_input_event_get_pointer_event(iev);

                    EXPECT_THAT(
                        mir_pointer_event_axis_value(pointer, mir_pointer_axis_relative_x),
                        FloatNear(event_x, 0.5f));
                    EXPECT_THAT(
                        mir_pointer_event_axis_value(pointer, mir_pointer_axis_relative_y),
                        FloatNear(event_y, 0.5f));

                    pointer_event_seen.raise();
                }
            }
        }

        bool wait_for_matching_input(std::chrono::seconds timeout)
        {
            return pointer_event_seen.wait_for(timeout);
        };

    private:
        mt::Signal pointer_event_seen;
    };

    auto listener = std::make_shared<DispatchEventListener>(1);
    server.the_seat_observer_registrar()->register_interest(listener);

    auto fake_pointer = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"name", "uid", mi::DeviceCapability::pointer});

    listener->wait_for_expected_devices(30s);

    fake_pointer->emit_event(
        mis::a_pointer_event()
            .with_movement(event_x, event_y));

    EXPECT_TRUE(listener->wait_for_matching_input(30s));
}
