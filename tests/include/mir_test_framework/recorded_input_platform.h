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

#ifndef MIR_TEST_FRAMEWORK_RECORDED_INPUT_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_RECORDED_INPUT_PLATFORM_H_

#include <mir_test_framework/udev_environment.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct MirEvent;

namespace mir
{
namespace input
{
class InputDevice;
}
}

namespace mir_test_framework
{
/**
 * Drives a real input platform module (eg: "input-evdev") against umockdev devices,
 * replaying evemu recordings and capturing the MirEvents the platform produces.
 *
 * Must be used in a process running with libumockdev-preload.
 */
class RecordedInputPlatform
{
public:
    using Events = std::vector<std::shared_ptr<MirEvent const>>;
    using Devices = std::vector<std::shared_ptr<mir::input::InputDevice>>;

    static std::chrono::milliseconds const default_timeout;

    /// Whether the named input platform module was built
    static auto module_available(std::string const& module_name) -> bool;

    explicit RecordedInputPlatform(std::string const& module_name);
    ~RecordedInputPlatform();

    /// Adds udev-recordings/<name>.umockdev (and .ioctl, if present). Call before start().
    void add_device(std::string const& name);

    /// Creates and starts the platform on a dedicated input thread
    void start();

    /// Stops and restarts the same platform instance on the input thread
    void restart();

    /// Waits for the platform to register at least count devices; returns those registered
    auto wait_for_devices(std::size_t count, std::chrono::milliseconds timeout = default_timeout) -> Devices;

    /// Runs action on the input thread (as the server does for device configuration) and waits for it
    void run_on_input_thread(std::function<void()> const& action);

    /// Replays udev-recordings/<name>.evemu into its device and waits for the replay to be written
    void replay(std::string const& name);

    /**
     * Waits until done(events) is satisfied or timeout expires
     *
     * \return all events captured so far
     */
    auto wait_for_events(
        std::function<bool(Events const&)> const& done,
        std::chrono::milliseconds timeout = default_timeout) -> Events;

    /// Waits until at least count events have been captured; returns all events captured so far
    auto wait_for_events(std::size_t count, std::chrono::milliseconds timeout = default_timeout) -> Events;

    /// Waits for the input thread to go idle, then returns all events captured so far
    auto settled_events(std::chrono::milliseconds quiet_period = std::chrono::milliseconds{100}) -> Events;

    UdevEnvironment udev;

private:
    struct Self;
    std::unique_ptr<Self> const self;
};

/**
 * A description of an input event for comparing platform output
 *
 * Omits details (timestamps, device ids) that legitimately differ between runs.
 */
auto describe_input_event(MirEvent const& event) -> std::string;
}

#endif // MIR_TEST_FRAMEWORK_RECORDED_INPUT_PLATFORM_H_
