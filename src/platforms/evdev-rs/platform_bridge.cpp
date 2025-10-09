/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform_bridge.h"
#include "platform.h"

#include "mir/console_services.h"
#include "mir/log.h"
#include "mir/fd.h"

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;

miers::PlatformBridgeC::PlatformBridgeC(
    Platform* platform,
    std::shared_ptr<mir::ConsoleServices> const& console)
    : platform(platform), console(console) {}

std::unique_ptr<miers::DeviceBridgeC> miers::PlatformBridgeC::acquire_device(int major, int minor) const
{
    mir::log_info("Acquiring device: %d.%d", major, minor);
    auto observer = platform->create_device_observer();
    DeviceObserverWithFd* raw_observer = observer.get();
    auto future = console->acquire_device(major, minor, std::move(observer));
    future.wait();
    return std::make_unique<DeviceBridgeC>(future.get(), raw_observer->raw_fd().value());
}

std::shared_ptr<mi::InputDevice> miers::PlatformBridgeC::create_input_device(int device_id) const
{
    return platform->create_input_device(device_id);
}

std::unique_ptr<miers::EventBuilderWrapper> miers::PlatformBridgeC::create_event_builder_wrapper(EventBuilder* event_builder) const
{
    return std::make_unique<EventBuilderWrapper>(event_builder);
}

miers::DeviceBridgeC::DeviceBridgeC(std::unique_ptr<mir::Device> device, int fd)
    : device(std::move(device)),
      fd(fd)
{
}

int miers::DeviceBridgeC::raw_fd() const
{
    return fd;
}

miers::EventBuilderWrapper::EventBuilderWrapper(EventBuilder* event_builder)
    : event_builder(event_builder)
{
}

mir::EventUPtr miers::EventBuilderWrapper::pointer_event(
    std::optional<uint64_t> time_nanoseconds,
    MirPointerAction action,
    MirPointerButtons buttons,
    std::optional<mir::geometry::PointF> position,
    mir::geometry::DisplacementF motion,
    MirPointerAxisSource axis_source,
    events::ScrollAxisH h_scroll,
    events::ScrollAxisV v_scroll)
{
    return event_builder->pointer_event(
        time_nanoseconds
            ? std::chrono::nanoseconds(*time_nanoseconds)
            : std::optional<EventBuilder::Timestamp>(std::nullopt),
        action,
        buttons,
        position,
        motion,
        axis_source,
        h_scroll,
        v_scroll
    );
}

