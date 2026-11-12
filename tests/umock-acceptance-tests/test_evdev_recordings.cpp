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

#include <mir_test_framework/recorded_input_platform.h>

#include <mir/events/event.h>
#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>
#include <mir/fd.h>
#include <mir/input/device_capability.h>
#include <mir/input/input_device.h>
#include <mir/input/input_device_info.h>
#include <mir/input/pointer_settings.h>
#include <mir/test/event_matchers.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <tuple>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

using namespace testing;

namespace
{
auto const keyboard = "thinkpad-keyboard";
auto const trackpoint = "thinkpad-trackpoint";
auto const touchpad = "thinkpad-touchpad";

// Originate from thinkpad-trackpoint-motion.evemu: 10 reports of REL_X=3, REL_Y=-2
int const motion_reports = 10;
float const motion_dx = 3;
float const motion_dy = -2;

auto key_events(mtf::RecordedInputPlatform::Events const& events) -> std::vector<MirKeyboardEvent const*>
{
    std::vector<MirKeyboardEvent const*> result;
    for (auto const& event : events)
    {
        if (event->type() == mir_event_type_input && event->to_input()->input_type() == mir_input_event_type_key)
        {
            result.push_back(event->to_input()->to_keyboard());
        }
    }
    return result;
}

auto pointer_events(mtf::RecordedInputPlatform::Events const& events, MirPointerAction action)
    -> std::vector<MirPointerEvent const*>
{
    std::vector<MirPointerEvent const*> result;
    for (auto const& event : events)
    {
        if (event->type() == mir_event_type_input &&
            event->to_input()->input_type() == mir_input_event_type_pointer &&
            event->to_input()->to_pointer()->action() == action)
        {
            result.push_back(event->to_input()->to_pointer());
        }
    }
    return result;
}

MATCHER_P2(IsKey, action, scan_code, "")
{
    return arg->action() == action && arg->scan_code() == scan_code;
}

auto near(float lhs, float rhs) -> bool
{
    return std::abs(lhs - rhs) <= 1e-3f;
}

auto near(mir::geometry::PointF lhs, mir::geometry::PointF rhs) -> bool
{
    return near(lhs.x.as_value(), rhs.x.as_value()) && near(lhs.y.as_value(), rhs.y.as_value());
}

auto near(std::optional<mir::geometry::PointF> const& lhs, std::optional<mir::geometry::PointF> const& rhs) -> bool
{
    return lhs && rhs ? near(*lhs, *rhs) : lhs.has_value() == rhs.has_value();
}

template<typename Tag>
auto near(mir::events::ScrollAxis<Tag> const& lhs, mir::events::ScrollAxis<Tag> const& rhs) -> bool
{
    return near(lhs.precise.as_value(), rhs.precise.as_value()) &&
        lhs.discrete == rhs.discrete &&
        lhs.value120 == rhs.value120 &&
        lhs.stop == rhs.stop;
}

auto equivalent(MirKeyboardEvent const& lhs, MirKeyboardEvent const& rhs) -> bool
{
    return lhs.action() == rhs.action() && lhs.scan_code() == rhs.scan_code();
}

auto equivalent(MirPointerEvent const& lhs, MirPointerEvent const& rhs) -> bool
{
    return lhs.action() == rhs.action() &&
        lhs.buttons() == rhs.buttons() &&
        lhs.axis_source() == rhs.axis_source() &&
        near(lhs.position(), rhs.position()) &&
        near(lhs.motion().dx.as_value(), rhs.motion().dx.as_value()) &&
        near(lhs.motion().dy.as_value(), rhs.motion().dy.as_value()) &&
        near(lhs.h_scroll(), rhs.h_scroll()) &&
        near(lhs.v_scroll(), rhs.v_scroll());
}

auto equivalent(MirTouchEvent const& lhs, MirTouchEvent const& rhs) -> bool
{
    if (lhs.pointer_count() != rhs.pointer_count())
    {
        return false;
    }
    for (std::size_t i = 0; i != lhs.pointer_count(); ++i)
    {
        if (lhs.id(i) != rhs.id(i) ||
            lhs.action(i) != rhs.action(i) ||
            lhs.tool_type(i) != rhs.tool_type(i) ||
            !near(lhs.position(i), rhs.position(i)))
        {
            return false;
        }
    }
    return true;
}

/// Ignores details (timestamps, device ids) that legitimately differ between platforms
auto equivalent(MirEvent const& lhs, MirEvent const& rhs) -> bool
{
    if (lhs.type() != rhs.type() || lhs.type() != mir_event_type_input)
    {
        return lhs.type() == rhs.type();
    }

    auto const& lhs_input = *lhs.to_input();
    auto const& rhs_input = *rhs.to_input();
    if (lhs_input.input_type() != rhs_input.input_type())
    {
        return false;
    }

    switch (lhs_input.input_type())
    {
    case mir_input_event_type_key:
        return equivalent(*lhs_input.to_keyboard(), *rhs_input.to_keyboard());
    case mir_input_event_type_pointer:
        return equivalent(*lhs_input.to_pointer(), *rhs_input.to_pointer());
    case mir_input_event_type_touch:
        return equivalent(*lhs_input.to_touch(), *rhs_input.to_touch());
    default:
        return true;
    }
}

MATCHER(IsEquivalentEvent, "")
{
    return equivalent(*std::get<0>(arg), *std::get<1>(arg));
}

struct EvdevRecordings : TestWithParam<std::string>
{
    void SetUp() override
    {
        if (!mtf::RecordedInputPlatform::module_available(GetParam()))
        {
            GTEST_SKIP() << GetParam() << " was not built";
        }
        platform.emplace(GetParam());
    }

    auto start_with(std::string const& device) -> std::shared_ptr<mi::InputDevice>
    {
        platform->add_device(device);
        platform->start();
        auto const devices = platform->wait_for_devices(1);
        return devices.empty() ? nullptr : devices.front();
    }

    std::optional<mtf::RecordedInputPlatform> platform;
};
}

TEST_P(EvdevRecordings, devices_are_registered_before_they_produce_input)
{
    platform->add_device(keyboard);
    platform->add_device(trackpoint);
    platform->add_device(touchpad);
    platform->start();

    auto const devices = platform->wait_for_devices(3);

    std::vector<mi::InputDeviceInfo> infos;
    for (auto const& device : devices)
    {
        infos.push_back(device->get_device_info());
    }
    EXPECT_THAT(infos, UnorderedElementsAre(
        Field(&mi::InputDeviceInfo::name, Eq("AT Translated Set 2 keyboard")),
        Field(&mi::InputDeviceInfo::name, Eq("TPPS/2 Elan TrackPoint")),
        Field(&mi::InputDeviceInfo::name, Eq("SYNA801A:00 06CB:CEC6 Touchpad"))));
}

TEST_P(EvdevRecordings, devices_report_their_capabilities)
{
    platform->add_device(keyboard);
    platform->add_device(trackpoint);
    platform->add_device(touchpad);
    platform->start();

    auto const devices = platform->wait_for_devices(3);
    ASSERT_THAT(devices, SizeIs(3));
    for (auto const& device : devices)
    {
        auto const info = device->get_device_info();
        SCOPED_TRACE(info.name);
        if (info.name == "AT Translated Set 2 keyboard")
        {
            EXPECT_TRUE(contains(info.capabilities, mi::DeviceCapability::keyboard));
            EXPECT_TRUE(contains(info.capabilities, mi::DeviceCapability::alpha_numeric));
            EXPECT_FALSE(contains(info.capabilities, mi::DeviceCapability::pointer));
        }
        else if (info.name == "TPPS/2 Elan TrackPoint")
        {
            EXPECT_TRUE(contains(info.capabilities, mi::DeviceCapability::pointer));
            EXPECT_FALSE(contains(info.capabilities, mi::DeviceCapability::touchpad));
            EXPECT_FALSE(contains(info.capabilities, mi::DeviceCapability::keyboard));
        }
        else if (info.name == "SYNA801A:00 06CB:CEC6 Touchpad")
        {
            EXPECT_TRUE(contains(info.capabilities, mi::DeviceCapability::pointer));
            EXPECT_FALSE(contains(info.capabilities, mi::DeviceCapability::keyboard));
        }
        else
        {
            ADD_FAILURE() << "Unexpected input device: " << info.name;
        }
    }
}

TEST_P(EvdevRecordings, touchpad_reports_touchpad_capability)
{
    if (GetParam() == "input-evdev-rs")
    {
        GTEST_SKIP() << "evdev-rs does not yet support touchpads";
    }

    auto const device = start_with(touchpad);
    ASSERT_THAT(device, NotNull());

    auto const capabilities = device->get_device_info().capabilities;
    EXPECT_TRUE(contains(capabilities, mi::DeviceCapability::touchpad));
    EXPECT_TRUE(contains(capabilities, mi::DeviceCapability::pointer));
}

TEST_P(EvdevRecordings, typing_hello_produces_key_down_and_up_for_each_letter)
{
    ASSERT_THAT(start_with(keyboard), NotNull());

    platform->play("thinkpad-keyboard-hello");
    platform->wait_for_events([](auto const& events) { return key_events(events).size() >= 10; });

    EXPECT_THAT(key_events(platform->settled_events()), ElementsAre(
        IsKey(mir_keyboard_action_down, KEY_H), IsKey(mir_keyboard_action_up, KEY_H),
        IsKey(mir_keyboard_action_down, KEY_E), IsKey(mir_keyboard_action_up, KEY_E),
        IsKey(mir_keyboard_action_down, KEY_L), IsKey(mir_keyboard_action_up, KEY_L),
        IsKey(mir_keyboard_action_down, KEY_L), IsKey(mir_keyboard_action_up, KEY_L),
        IsKey(mir_keyboard_action_down, KEY_O), IsKey(mir_keyboard_action_up, KEY_O)));
}

TEST_P(EvdevRecordings, clicking_produces_primary_button_down_then_up)
{
    ASSERT_THAT(start_with(trackpoint), NotNull());

    platform->play("thinkpad-trackpoint-click");
    platform->wait_for_events(
        [](auto const& events) { return !pointer_events(events, mir_pointer_action_button_up).empty(); });
    auto const events = platform->settled_events();

    auto const downs = pointer_events(events, mir_pointer_action_button_down);
    auto const ups = pointer_events(events, mir_pointer_action_button_up);
    ASSERT_THAT(downs.size(), Eq(1u));
    ASSERT_THAT(ups.size(), Eq(1u));
    EXPECT_THAT(downs.front()->buttons(), Eq(mir_pointer_button_primary));
    EXPECT_THAT(ups.front()->buttons(), Eq(0u));
    EXPECT_THAT(pointer_events(events, mir_pointer_action_motion), IsEmpty());
}

TEST_P(EvdevRecordings, motion_with_default_acceleration_is_in_the_recorded_direction)
{
    ASSERT_THAT(start_with(trackpoint), NotNull());

    platform->play("thinkpad-trackpoint-motion");
    platform->wait_for_events(
        [](auto const& events) { return pointer_events(events, mir_pointer_action_motion).size() >= motion_reports; });

    auto const motion = pointer_events(platform->settled_events(), mir_pointer_action_motion);
    ASSERT_THAT(motion.size(), Eq(motion_reports));
    for (auto const* event : motion)
    {
        EXPECT_THAT(event->motion().dx.as_value(), Gt(0));
        EXPECT_THAT(event->motion().dy.as_value(), Lt(0));
        // The direction is preserved, even if the magnitude is accelerated
        EXPECT_THAT(event->motion().dy.as_value() / event->motion().dx.as_value(), FloatNear(motion_dy / motion_dx, 1e-3));
        EXPECT_THAT(event->buttons(), Eq(0u));
    }
}

TEST_P(EvdevRecordings, motion_with_flat_acceleration_reports_the_recorded_deltas)
{
    auto const device = start_with(trackpoint);
    ASSERT_THAT(device, NotNull());

    platform->run_on_input_thread(
        [&]
        {
            mi::PointerSettings flat;
            flat.acceleration = mir_pointer_acceleration_none;
            flat.cursor_acceleration_bias = 0;
            device->apply_settings(flat);
        });

    platform->play("thinkpad-trackpoint-motion");
    platform->wait_for_events(
        [](auto const& events) { return pointer_events(events, mir_pointer_action_motion).size() >= motion_reports; });

    auto const motion = pointer_events(platform->settled_events(), mir_pointer_action_motion);
    ASSERT_THAT(motion.size(), Eq(motion_reports));
    // libinput's flat profile applies a constant factor that isn't exactly 1 (eg: trackpoint range normalisation)
    float const tolerance = 0.01;
    for (auto const* event : motion)
    {
        EXPECT_THAT(event->motion().dx.as_value(), FloatNear(motion_dx, motion_dx * tolerance));
        EXPECT_THAT(event->motion().dy.as_value(), FloatNear(motion_dy, -motion_dy * tolerance));
        // Unlike adaptive acceleration, the first (slow) report is scaled the same as the rest
        EXPECT_THAT(event->motion().dx.as_value(), FloatEq(motion.front()->motion().dx.as_value()));
        EXPECT_THAT(event->motion().dy.as_value(), FloatEq(motion.front()->motion().dy.as_value()));
    }
}

INSTANTIATE_TEST_SUITE_P(, EvdevRecordings, Values("input-evdev", "input-evdev-rs"));

TEST(EvdevPlatformParity, platforms_produce_the_same_events_from_the_same_recordings)
{
    std::vector<std::string> const modules{"input-evdev", "input-evdev-rs"};
    for (auto const& module : modules)
    {
        if (!mtf::RecordedInputPlatform::module_available(module))
        {
            GTEST_SKIP() << module << " was not built";
        }
    }

    for (auto const& [device, recording] : {
        std::pair{keyboard, "thinkpad-keyboard-hello"},
        std::pair{trackpoint, "thinkpad-trackpoint-click"},
        std::pair{trackpoint, "thinkpad-trackpoint-motion"}})
    {
        SCOPED_TRACE(recording);
        std::vector<mtf::RecordedInputPlatform::Events> platform_events;
        for (auto const& module : modules)
        {
            mtf::RecordedInputPlatform platform{module};
            platform.add_device(device);
            platform.start();
            auto const devices = platform.wait_for_devices(1);
            ASSERT_THAT(devices.size(), Eq(1u)) << module;

            // Adaptive acceleration isn't reproducible to the last digit, so compare unaccelerated motion
            platform.run_on_input_thread(
                [&]
                {
                    if (devices.front()->get_pointer_settings())
                    {
                        mi::PointerSettings flat;
                        flat.acceleration = mir_pointer_acceleration_none;
                        devices.front()->apply_settings(flat);
                    }
                });

            platform.play(recording);
            platform.wait_for_events(1);
            platform_events.push_back(platform.settled_events());
        }
        EXPECT_THAT(platform_events[0], Not(IsEmpty()));
        EXPECT_THAT(platform_events[1], Pointwise(IsEquivalentEvent(), platform_events[0]));
    }
}
