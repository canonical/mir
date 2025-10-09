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

#ifndef MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H
#define MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H

#include <memory>
#include "mir/console_services.h"
#include "mir/input/input_device.h"
#include "mir/input/event_builder.h"
#include "mir/geometry/displacement.h"

namespace mir
{
class Device;

namespace input
{
namespace evdev_rs
{
class Platform;

/// Wraps a #mir::Device so that it can be kept alive in rust.
///
/// The platform must maintain a reference to the #mir::Device
/// or else the device will be destroyed.
class DeviceBridgeC
{
public:
    DeviceBridgeC(std::unique_ptr<Device> device, int fd);
    int raw_fd() const;

private:
    std::unique_ptr<Device> device;
    int fd;
};

class EventBuilderWrapper
{
public:
    explicit EventBuilderWrapper(EventBuilder* event_builder);
    EventUPtr pointer_event(
        std::optional<uint64_t> time_nanoseconds,
        MirPointerAction action,
        MirPointerButtons buttons,
        std::optional<mir::geometry::PointF> position,
        mir::geometry::DisplacementF motion,
        MirPointerAxisSource axis_source,
        events::ScrollAxisH h_scroll,
        events::ScrollAxisV v_scroll
    );

private:
    EventBuilder* event_builder;
};

/// Bridge allowing conversation from Rust to C++.
///
/// The rust code may use this bridge to request things from the system.
class PlatformBridgeC
{
public:
    PlatformBridgeC(Platform* platform, std::shared_ptr<mir::ConsoleServices> const& console);
    virtual ~PlatformBridgeC() = default;
    std::unique_ptr<DeviceBridgeC> acquire_device(int major, int minor) const;
    std::shared_ptr<InputDevice> create_input_device(int device_id) const;
    std::unique_ptr<EventBuilderWrapper> create_event_builder_wrapper(EventBuilder* event_builder) const;

private:
    Platform* platform;
    std::shared_ptr<mir::ConsoleServices> console;
};
}
}
}

#endif // MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H
