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
#include <mir/fd.h>
#include <mir/input/device_capability.h>
#include <mir/input/input_device.h>
#include <mir/input/input_device_info.h>
#include <mir/input/pointer_settings.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <ranges>
#include <system_error>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

using namespace testing;

namespace
{
auto const keyboard = "thinkpad-keyboard";
auto const trackpoint = "thinkpad-trackpoint";
auto const touchpad = "thinkpad-touchpad";

// thinkpad-trackpoint-motion.evemu: 10 reports of REL_X=3, REL_Y=-2
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

TEST(RecordedInputReplayDeathTest, reports_failure_to_write_rebased_recording)
{
    ASSERT_EXIT(
        {
            auto const result = []
            {
                mtf::UdevEnvironment udev;
                udev.add_standard_device(trackpoint);
                struct rlimit limit{};
                if (getrlimit(RLIMIT_FSIZE, &limit) != 0)
                {
                    return 2;
                }
                limit.rlim_cur = 0;
                if (signal(SIGXFSZ, SIG_IGN) == SIG_ERR || setrlimit(RLIMIT_FSIZE, &limit) != 0)
                {
                    return 2;
                }
                try
                {
                    udev.load_device_evemu("thinkpad-trackpoint-click");
                }
                catch (std::system_error const& error)
                {
                    return error.code().value() == EFBIG ? 0 : 1;
                }
                return 1;
            }();
            _exit(result);
        },
        ExitedWithCode(0), "");
}

TEST(RecordedInputFds, duplicated_devices_retain_ioctl_emulation)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device(trackpoint);
    mir::Fd fd{open("/dev/input/event4", O_RDWR | O_NONBLOCK | O_CLOEXEC)};
    ASSERT_THAT(fd, Ne(mir::Fd::invalid));
    mtf::reopen_on_dup(fd, "/dev/input/event4");

    // evdev-rs first calls dup() in C++, then fcntl(F_DUPFD_CLOEXEC) in OwnedFd::try_clone().
    mir::Fd first{dup(fd)};
    ASSERT_THAT(first, Ne(mir::Fd::invalid));
    input_id id{};
    EXPECT_THAT(ioctl(first, EVIOCGID, &id), Eq(0));
    EXPECT_THAT(id.vendor, Eq(2));
    EXPECT_THAT(fcntl(first, F_GETFD), Eq(0));
    EXPECT_THAT(fcntl(first, F_GETFL) & O_NONBLOCK, Ne(0));

    for (auto const* symbol : {"fcntl", "fcntl64"})
    {
        SCOPED_TRACE(symbol);
        auto const call = reinterpret_cast<int (*)(int, int, ...)>(dlsym(RTLD_DEFAULT, symbol));
        ASSERT_THAT(call, NotNull());
        for (auto const command : {F_DUPFD, F_DUPFD_CLOEXEC})
        {
            SCOPED_TRACE(command);
            mir::Fd copy{call(first, command, 0)};
            ASSERT_THAT(copy, Ne(mir::Fd::invalid));
            input_id copied_id{};
            EXPECT_THAT(ioctl(copy, EVIOCGID, &copied_id), Eq(0));
            EXPECT_THAT(copied_id.vendor, Eq(2));
            EXPECT_THAT(fcntl(copy, F_GETFD), Eq(command == F_DUPFD_CLOEXEC ? FD_CLOEXEC : 0));
            EXPECT_THAT(fcntl(copy, F_GETFL) & O_NONBLOCK, Ne(0));
        }
    }
}

TEST(RecordedInputFds, invalid_or_unsatisfied_minimum_is_not_ignored)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device(trackpoint);
    mir::Fd fd{open("/dev/input/event4", O_RDWR)};
    ASSERT_THAT(fd, Ne(mir::Fd::invalid));
    mtf::reopen_on_dup(fd, "/dev/input/event4");

    for (auto const* symbol : {"fcntl", "fcntl64"})
    {
        SCOPED_TRACE(symbol);
        auto const call = reinterpret_cast<int (*)(int, int, ...)>(dlsym(RTLD_DEFAULT, symbol));
        ASSERT_THAT(call, NotNull());
        for (auto const command : {F_DUPFD, F_DUPFD_CLOEXEC})
        {
            SCOPED_TRACE(command);
            auto const invalid = call(fd, command, -1);
            auto const invalid_error = errno;
            EXPECT_THAT(invalid, Eq(-1));
            EXPECT_THAT(invalid_error, Eq(EINVAL));

            auto const minimum = fd + 100;
            mir::Fd copy{call(fd, command, minimum)};
            auto const copy_error = errno;
            if (copy == mir::Fd::invalid)
            {
                EXPECT_THAT(copy_error, Eq(EOPNOTSUPP));
            }
            else
            {
                EXPECT_THAT(copy, Ge(minimum));
            }
        }
    }
}

TEST(RecordedInputFds, unrelated_descriptors_keep_native_duplication_and_fcntl_behaviour)
{
    std::unique_ptr<std::FILE, int (*)(std::FILE*)> file{std::tmpfile(), &std::fclose};
    ASSERT_THAT(file, NotNull());
    auto const fd = fileno(file.get());
    ASSERT_THAT(lseek(fd, 7, SEEK_SET), Eq(7));

    mir::Fd copy{dup(fd)};
    ASSERT_THAT(copy, Ne(mir::Fd::invalid));
    EXPECT_THAT(lseek(copy, 0, SEEK_CUR), Eq(7));
    EXPECT_THAT(lseek(copy, 3, SEEK_SET), Eq(3));
    EXPECT_THAT(lseek(fd, 0, SEEK_CUR), Eq(3));

    for (auto const* symbol : {"fcntl", "fcntl64"})
    {
        SCOPED_TRACE(symbol);
        auto const call = reinterpret_cast<int (*)(int, int, ...)>(dlsym(RTLD_DEFAULT, symbol));
        ASSERT_THAT(call, NotNull());
        EXPECT_THAT(call(fd, F_SETFD, FD_CLOEXEC), Eq(0));
        EXPECT_THAT(call(fd, F_GETFD), Eq(FD_CLOEXEC));
        EXPECT_THAT(call(fd, F_SETFD, 0), Eq(0));
        EXPECT_THAT(call(fd, F_GETFD), Eq(0));

        struct flock lock{};
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        EXPECT_THAT(call(fd, F_GETLK, &lock), Eq(0));
        EXPECT_THAT(lock.l_type, Eq(F_UNLCK));

        mir::Fd high_fd{call(fd, F_DUPFD_CLOEXEC, 128)};
        ASSERT_THAT(high_fd, Ne(mir::Fd::invalid));
        EXPECT_THAT(high_fd, Ge(128));
        EXPECT_THAT(call(high_fd, F_GETFD), Eq(FD_CLOEXEC));
        EXPECT_THAT(lseek(high_fd, 0, SEEK_CUR), Eq(3));
    }
}

TEST(RecordedInputFds, closed_registration_does_not_redirect_reused_descriptor)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device(trackpoint);
    std::unique_ptr<std::FILE, int (*)(std::FILE*)> file{std::tmpfile(), &std::fclose};
    ASSERT_THAT(file, NotNull());

    int registered_fd;
    {
        mir::Fd fd{open("/dev/input/event4", O_RDWR)};
        ASSERT_THAT(fd, Ne(mir::Fd::invalid));
        mtf::reopen_on_dup(fd, "/dev/input/event4");
        registered_fd = fd;
    }
    mir::Fd reused{dup2(fileno(file.get()), registered_fd)};
    ASSERT_THAT(reused, Eq(registered_fd));
    ASSERT_THAT(lseek(reused, 7, SEEK_SET), Eq(7));

    mir::Fd copy{dup(reused)};
    ASSERT_THAT(copy, Ne(mir::Fd::invalid));
    EXPECT_THAT(lseek(copy, 0, SEEK_CUR), Eq(7));
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

    platform->replay("thinkpad-keyboard-hello");
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

    platform->replay("thinkpad-trackpoint-click");
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

    platform->replay("thinkpad-trackpoint-motion");
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

    platform->replay("thinkpad-trackpoint-motion");
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
        std::vector<std::vector<std::string>> descriptions;
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

            platform.replay(recording);
            platform.wait_for_events(1);
            auto const events = platform.settled_events();
            auto const described = events | std::views::transform([](auto const& e) { return mtf::describe_input_event(*e); });
            descriptions.emplace_back(described.begin(), described.end());
        }
        EXPECT_THAT(descriptions[0], Not(IsEmpty()));
        EXPECT_THAT(descriptions[1], ContainerEq(descriptions[0]));
    }
}
