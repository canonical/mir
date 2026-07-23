/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "virtual_pointer_v1.h"
#include "wayland.h"
#include "wl_pointer.h"
#include "output_manager.h"

#include <mir/events/pointer_event.h>
#include <mir/executor.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/device.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/input/virtual_input_device.h>
#include <mir/log.h>
#include "weak.h"

#include "protocol_error.h"

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mwrs = mir::wayland;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mev = mir::events;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{
struct VirtualPointerV1Ctx
{
    OutputManager* const output_manager;
    std::shared_ptr<mi::InputDeviceRegistry> const device_registry;
};

class VirtualPointerManagerV1
    : public wayland::VirtualPointerManagerV1
{
public:
    VirtualPointerManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::VirtualPointerManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    using wayland::VirtualPointerManagerV1::create_virtual_pointer;
    auto create_virtual_pointer(
        std::optional<wayland::Weak<wayland::Seat>> const& seat,
        rust::Box<wayland::VirtualPointerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::VirtualPointerV1> override;
    using wayland::VirtualPointerManagerV1::create_virtual_pointer_with_output;
    auto create_virtual_pointer_with_output(
        std::optional<wayland::Weak<wayland::Seat>> const& seat,
        std::optional<wayland::Weak<wayland::Output>> const& output,
        rust::Box<wayland::VirtualPointerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::VirtualPointerV1> override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};

class VirtualPointerV1
    : public wayland::VirtualPointerV1
{
public:
    VirtualPointerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::VirtualPointerV1Middleware> instance,
        uint32_t object_id,
        std::optional<wayland::Weak<wayland::Output>> output,
        std::shared_ptr<VirtualPointerV1Ctx> const& ctx);
    ~VirtualPointerV1();

private:
    void motion(uint32_t time, double dx, double dy) override;
    void motion_absolute(uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent) override;
    void button(uint32_t time, uint32_t button, uint32_t state) override;
    void axis(uint32_t time, uint32_t axis, double value) override;
    void frame() override;
    void axis_source(uint32_t axis_source) override;
    void axis_stop(uint32_t time, uint32_t axis) override;
    void axis_discrete(uint32_t time, uint32_t axis, double value, int32_t discrete) override;

    void update_absolute_motion_area();

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
    std::shared_ptr<input::VirtualInputDevice> const pointer_device;
    mw::Weak<OutputGlobal> const output;

    struct Pending
    {
        Pending(geometry::PointF position, MirPointerButtons buttons_pressed)
            : position{position},
              buttons_pressed{buttons_pressed}
        {
        }

        std::optional<mi::EventBuilder::Timestamp> timestamp;
        bool has_absolute_motion{false};
        geometry::PointF position;
        MirPointerAxisSource axis_source{mir_pointer_axis_source_none};
        events::ScrollAxisH scroll_h;
        events::ScrollAxisV scroll_v;
        MirPointerButtons buttons_pressed;
    };
    static void create_and_dispatch_event(
        input::VirtualInputDevice* pointer_device,
        Pending pending,
        geometry::PointF position,
        MirPointerButtons buttons_pressed);

    geometry::Rectangle absolute_motion_area;
    MirPointerButtons buttons_pressed{0};
    geometry::PointF position;
    Pending pending{position, buttons_pressed};
};
}
}

auto mf::create_virtual_pointer_manager_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::VirtualPointerManagerV1Middleware> instance,
    uint32_t object_id,
    OutputManager* output_manager,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<wayland::VirtualPointerManagerV1>
{
    auto ctx = std::shared_ptr<VirtualPointerV1Ctx>{new VirtualPointerV1Ctx{output_manager, device_registry}};
    return std::make_shared<VirtualPointerManagerV1>(std::move(client), std::move(instance), object_id, std::move(ctx));
}

mf::VirtualPointerManagerV1::VirtualPointerManagerV1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::VirtualPointerManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerManagerV1{std::move(client), std::move(instance), object_id},
      ctx{ctx}
{
}

auto mf::VirtualPointerManagerV1::create_virtual_pointer(
    std::optional<wayland::Weak<wayland::Seat>> const&,
    rust::Box<wayland::VirtualPointerV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<wayland::VirtualPointerV1>
{
    return std::make_shared<VirtualPointerV1>(client, std::move(child_instance), child_object_id, std::nullopt, ctx);
}

auto mf::VirtualPointerManagerV1::create_virtual_pointer_with_output(
    std::optional<wayland::Weak<wayland::Seat>> const&,
    std::optional<wayland::Weak<wayland::Output>> const& output,
    rust::Box<wayland::VirtualPointerV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<wayland::VirtualPointerV1>
{
    return std::make_shared<VirtualPointerV1>(client, std::move(child_instance), child_object_id, output, ctx);
}


mf::VirtualPointerV1::VirtualPointerV1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::VirtualPointerV1Middleware> instance,
    uint32_t object_id,
    std::optional<wayland::Weak<wayland::Output>> output,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerV1{std::move(client), std::move(instance), object_id},
      ctx{ctx},
      pointer_device{std::make_shared<mi::VirtualInputDevice>("virtual-pointer", mi::DeviceCapability::pointer)},
      output{ctx->output_manager->output_for(
          OutputManager::output_id_for(output ? mwrs::as_nullable_ptr(output.value()) : nullptr).value_or(
              mg::DisplayConfigurationOutputId{})).value_or(nullptr)}
{
    update_absolute_motion_area();
    ctx->device_registry->add_device(pointer_device);
}

mf::VirtualPointerV1::~VirtualPointerV1()
{
    // Destruction must happen on the linearising_executor to prevent races with create_and_dispatch_event
    mir::linearising_executor.spawn(
        [pointer_device=pointer_device, device_registry=ctx->device_registry]()
    {
        device_registry->remove_device(pointer_device);
    });
}

void mf::VirtualPointerV1::motion(uint32_t time, double dx, double dy)
{
    pending.timestamp = std::chrono::milliseconds{time};
    pending.position += geom::DisplacementF{dx, dy};
    // Leave has_absolute_motion true if we previously got a .motion_absolute request in this frame
}

void mf::VirtualPointerV1::motion_absolute(uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent)
{
    if (x > x_extent || y > y_extent)
    {
        // FIXME: Specific error proposed in https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/merge_requests/143
        throw mwrs::ProtocolError{object_id(), 0, "Absolute motion coordinates %u,%u out of bounds %u,%u", x, y, x_extent, y_extent};
    }

    pending.timestamp = std::chrono::milliseconds{time};
    pending.position = geom::PointF{absolute_motion_area.top_left};
    if (x_extent > 0)
    {
        pending.position += geom::DeltaXF{x} * absolute_motion_area.size.width.as_value() / x_extent;
    }
    if (y_extent > 0)
    {
        pending.position += geom::DeltaYF{y} * absolute_motion_area.size.height.as_value() / y_extent;
    }
    pending.has_absolute_motion = true;
}

void mf::VirtualPointerV1::button(uint32_t time, uint32_t button, uint32_t state)
{
    pending.timestamp = std::chrono::milliseconds{time};
    if (auto const mir_button = WlPointer::linux_button_to_mir_button(button))
    {
        switch (state)
        {
        case mwrs::Pointer::ButtonState::pressed:
            pending.buttons_pressed |= mir_button.value();
            break;

        case mwrs::Pointer::ButtonState::released:
            pending.buttons_pressed &= ~mir_button.value();
            break;

        default:
            throw mwrs::ProtocolError{object_id(), 0, "Invalid button state %d", state};
        }
    }
    else
    {
        // Since the set of allowed buttons is not clearly defined, we warn instead of throwing a protocol error
        log_warning("zwlr_virtual_pointer_v1.button given unknown button %d", button);
    }
}

void mf::VirtualPointerV1::axis(uint32_t time, uint32_t axis, double value)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mwrs::Pointer::Axis::horizontal_scroll:
        pending.scroll_h.precise += geom::DeltaXF{value};
        break;

    case mwrs::Pointer::Axis::vertical_scroll:
        pending.scroll_v.precise += geom::DeltaYF{value};
        break;

    default:
        throw mwrs::ProtocolError{object_id(), Error::invalid_axis, "Unknown axis %d", axis};
    }
}

void mf::VirtualPointerV1::frame()
{
    mir::linearising_executor.spawn(
        [pointer_device=pointer_device, pending=pending, position=position, buttons_pressed=buttons_pressed]
        { create_and_dispatch_event(pointer_device.get(), pending, position, buttons_pressed); });

    buttons_pressed = pending.buttons_pressed;
    position = pending.position;
    pending = Pending{position, buttons_pressed};
}

void mf::VirtualPointerV1::create_and_dispatch_event(
    input::VirtualInputDevice* pointer_device,
    Pending const pending,
    geometry::PointF const position,
    MirPointerButtons const buttons_pressed)
{
    pointer_device->if_started_then([&](input::InputSink* sink, input::EventBuilder* builder)
        {
            if (pending.has_absolute_motion ||
                position != pending.position ||
                pending.scroll_h != mev::ScrollAxisH{} ||
                pending.scroll_v != mev::ScrollAxisV{})
            {
                auto const delta = pending.position - position;
                auto const absolute_motion = pending.has_absolute_motion ?
                    std::make_optional(pending.position) :
                    std::nullopt;
                auto event = builder->pointer_event(
                    pending.timestamp,
                    mir_pointer_action_motion,
                    buttons_pressed,
                    absolute_motion,
                    delta,
                    pending.axis_source,
                    pending.scroll_h,
                    pending.scroll_v);
                sink->handle_input(std::move(event));
            }

            auto const pressed_buttons = pending.buttons_pressed & ~buttons_pressed;
            auto const released_buttons = ~pending.buttons_pressed & buttons_pressed;

            if (released_buttons)
            {
                auto const buttons_after_release = pending.buttons_pressed & buttons_pressed;
                sink->handle_input(builder->pointer_event(
                    pending.timestamp,
                    mir_pointer_action_button_up,
                    buttons_after_release,
                    {}, {}, mir_pointer_axis_source_none, {}, {}));
            }

            if (pressed_buttons)
            {
                sink->handle_input(builder->pointer_event(
                    pending.timestamp,
                    mir_pointer_action_button_down,
                    pending.buttons_pressed,
                    {}, {}, mir_pointer_axis_source_none, {}, {}));
            }
        });
}

void mf::VirtualPointerV1::axis_source(uint32_t axis_source)
{
    switch (axis_source)
    {
    case mwrs::Pointer::AxisSource::wheel: pending.axis_source = mir_pointer_axis_source_wheel; break;
    case mwrs::Pointer::AxisSource::finger: pending.axis_source = mir_pointer_axis_source_finger; break;
    case mwrs::Pointer::AxisSource::continuous: pending.axis_source = mir_pointer_axis_source_continuous; break;
    case mwrs::Pointer::AxisSource::wheel_tilt: pending.axis_source = mir_pointer_axis_source_wheel_tilt; break;
    default:
        throw mwrs::ProtocolError{object_id(), Error::invalid_axis_source, "Unknown axis source %d", axis_source};
    }
}

void mf::VirtualPointerV1::axis_stop(uint32_t time, uint32_t axis)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mwrs::Pointer::Axis::horizontal_scroll: pending.scroll_h.stop = true; break;
    case mwrs::Pointer::Axis::vertical_scroll: pending.scroll_v.stop = true; break;
    default:
        throw mwrs::ProtocolError{object_id(), Error::invalid_axis, "Unknown axis %d", axis};
    }
}

void mf::VirtualPointerV1::axis_discrete(uint32_t time, uint32_t axis, double value, int32_t discrete)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mwrs::Pointer::Axis::horizontal_scroll:
        pending.scroll_h.discrete += geom::DeltaX{discrete};
        pending.scroll_h.value120 += geom::DeltaX{discrete * 120};
        pending.scroll_h.precise += geom::DeltaXF{value};
        break;

    case mwrs::Pointer::Axis::vertical_scroll:
        pending.scroll_v.discrete += geom::DeltaY{discrete};
        pending.scroll_v.value120 += geom::DeltaY{discrete * 120};
        pending.scroll_v.precise += geom::DeltaYF{value};
        break;

    default:
        throw mwrs::ProtocolError{object_id(), Error::invalid_axis, "Unknown axis %d", axis};
    }
}

void mf::VirtualPointerV1::update_absolute_motion_area()
{
    if (output)
    {
        absolute_motion_area = output.value().current_config().extents();
    }
    else
    {
        // Set absolute_motion_area to the bounding box of all outputs
        bool first_output = true;
        ctx->output_manager->current_config().for_each_output([&](mg::DisplayConfigurationOutput const& output)
            {
                if (first_output)
                {
                    absolute_motion_area = output.extents();
                    first_output = false;
                }
                else
                {
                    absolute_motion_area = geom::Rectangles{
                        absolute_motion_area,
                        output.extents()}.bounding_rectangle();
                }
            });
    }
}
