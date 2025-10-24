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
#include "mir/input/input_sink.h"
#include "mir/geometry/forward.h"

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;
namespace geom = mir::geometry;

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

std::unique_ptr<miers::RectangleC> miers::PlatformBridgeC::bounding_rectangle(InputSink const& input_sink) const
{
    auto const rectangle = input_sink.bounding_rectangle();
    return std::make_unique<RectangleC>(
        rectangle.top_left.x.as_int(),
        rectangle.top_left.y.as_int(),
        rectangle.size.width.as_int(),
        rectangle.size.height.as_int()
    );
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

std::shared_ptr<MirEvent> miers::EventBuilderWrapper::pointer_event(
    bool has_time,
    uint64_t time_microseconds,
    int32_t action,
    uint32_t buttons,
    bool has_position,
    float position_x,
    float position_y,
    float displacement_x,
    float displacement_y,
    int32_t axis_source,
    float precise_x,
    int discrete_x,
    int value120_x,
    bool scroll_stop_x,
    float precise_y,
    int discrete_y,
    int value120_y,
    bool scroll_stop_y) const
{
    return event_builder->pointer_event(
        has_time
            ? std::chrono::microseconds(time_microseconds)
            : std::optional<EventBuilder::Timestamp>(std::nullopt),
        static_cast<MirPointerAction>(action),
        buttons,
        has_position
            ? geometry::PointF(position_x, position_y)
            : std::optional<geometry::PointF>(std::nullopt),
        geometry::DisplacementF(displacement_x, displacement_y),
        static_cast<MirPointerAxisSource>(axis_source),
        events::ScrollAxisH(
            geom::generic::Value<float, geom::DeltaXTag>(precise_x),
            geom::generic::Value<int, geom::DeltaXTag>(discrete_x),
            geom::generic::Value<int, geom::DeltaXTag>(value120_x),
            scroll_stop_x),
        events::ScrollAxisV(
            geom::generic::Value<float, geom::DeltaYTag>(precise_y),
            geom::generic::Value<int, geom::DeltaYTag>(discrete_y),
            geom::generic::Value<int, geom::DeltaYTag>(value120_y),
            scroll_stop_y)
    );
}

std::shared_ptr<MirEvent> miers::EventBuilderWrapper::key_event(
        bool has_time,
        uint64_t time_microseconds,
        int32_t action,
        uint32_t scancode
    ) const
{
    return event_builder->key_event(
        has_time
            ? std::chrono::microseconds(time_microseconds)
            : std::optional<EventBuilder::Timestamp>(std::nullopt),
        static_cast<MirKeyboardAction>(action),
        xkb_keysym_t{0},
        static_cast<int>(scancode)
    );
}
