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

miers::PlatformBridge::PlatformBridge(
    Platform* platform,
    std::shared_ptr<mir::ConsoleServices> const& console)
    : platform(platform), console(console) {}

std::unique_ptr<miers::DeviceWrapper> miers::PlatformBridge::acquire_device(int major, int minor) const
{
    mir::log_info("Acquiring device: %d.%d", major, minor);
    auto observer = platform->create_device_observer();
    DeviceObserverWithFd* raw_observer = observer.get();
    auto future = console->acquire_device(major, minor, std::move(observer));
    future.wait();
    if (auto const raw_fd = raw_observer->raw_fd(); !raw_fd)
        throw std::runtime_error("Failed to acquire device");
    else
        return std::make_unique<DeviceWrapper>(future.get(), raw_fd.value());
}

std::shared_ptr<mi::InputDevice> miers::PlatformBridge::create_input_device(int device_id) const
{
    return platform->create_input_device(device_id);
}

std::unique_ptr<miers::EventBuilderWrapper> miers::PlatformBridge::create_event_builder_wrapper(EventBuilder* event_builder) const
{
    return std::make_unique<EventBuilderWrapper>(event_builder);
}

std::unique_ptr<miers::RectangleWrapper> miers::PlatformBridge::bounding_rectangle(InputSink const& input_sink) const
{
    return std::make_unique<RectangleWrapper>(input_sink.bounding_rectangle());
}

miers::DeviceWrapper::DeviceWrapper(std::unique_ptr<mir::Device> device, int fd)
    : device(std::move(device)),
      fd(fd)
{
}

int miers::DeviceWrapper::raw_fd() const
{
    return fd;
}

miers::EventBuilderWrapper::EventBuilderWrapper(EventBuilder* event_builder)
    : event_builder(event_builder)
{
}

std::shared_ptr<MirEvent> miers::EventBuilderWrapper::pointer_event(PointerEventData const& data) const
{
    return event_builder->pointer_event(
        data.has_time
            ? std::chrono::microseconds(data.time_microseconds)
            : std::optional<EventBuilder::Timestamp>(std::nullopt),
        static_cast<MirPointerAction>(data.action),
        data.buttons,
        data.has_position
            ? geometry::PointF(data.position_x, data.position_y)
            : std::optional<geometry::PointF>(std::nullopt),
        geometry::DisplacementF(data.displacement_x, data.displacement_y),
        static_cast<MirPointerAxisSource>(data.axis_source),
        events::ScrollAxisH(
            geom::generic::Value<float, geom::DeltaXTag>(data.precise_x),
            geom::generic::Value<int, geom::DeltaXTag>(data.discrete_x),
            geom::generic::Value<int, geom::DeltaXTag>(data.value120_x),
            data.scroll_stop_x),
        events::ScrollAxisV(
            geom::generic::Value<float, geom::DeltaYTag>(data.precise_y),
            geom::generic::Value<int, geom::DeltaYTag>(data.discrete_y),
            geom::generic::Value<int, geom::DeltaYTag>(data.value120_y),
            data.scroll_stop_y)
    );
}

std::shared_ptr<MirEvent> miers::EventBuilderWrapper::key_event(KeyEventData const& data) const
{
    return event_builder->key_event(
        data.has_time
            ? std::chrono::microseconds(data.time_microseconds)
            : std::optional<EventBuilder::Timestamp>(std::nullopt),
        static_cast<MirKeyboardAction>(data.action),
        xkb_keysym_t{0},
        static_cast<int>(data.scancode)
    );
}

std::shared_ptr<MirEvent> miers::EventBuilderWrapper::touch_event(TouchEventData const& data) const
{
    std::vector<events::TouchContact> contacts;
    for (auto const& contact : data.contacts)
    {
        contacts.push_back(events::TouchContact(
            static_cast<MirTouchId>(contact.touch_id),
            static_cast<MirTouchAction>(contact.action),
            static_cast<MirTouchTooltype>(contact.tooltype),
            geom::PointF(contact.position_x, contact.position_y),
            contact.pressure,
            contact.touch_major,
            contact.touch_minor,
            contact.orientation
        ));
    }
    return event_builder->touch_event(
        data.has_time
          ? std::chrono::microseconds(data.time_microseconds)
          : std::optional<EventBuilder::Timestamp>(std::nullopt),
          contacts
    );
}

miers::RectangleWrapper::RectangleWrapper(geometry::Rectangle&& rect)
    : rect{std::move(rect)}
{
}

int32_t miers::RectangleWrapper::x() const
{
    return rect.top_left.x.as_value();
}

int32_t miers::RectangleWrapper::y() const
{
    return rect.top_left.y.as_value();
}

int32_t miers::RectangleWrapper::width() const
{
    return rect.size.width.as_value();
}

int32_t miers::RectangleWrapper::height() const
{
    return rect.size.height.as_value();
}
