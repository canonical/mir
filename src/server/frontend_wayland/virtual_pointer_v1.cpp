/*
 * Copyright © 2021 Canonical Ltd.
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
#include "wayland_wrapper.h"
#include "wl_pointer.h"
#include "output_manager.h"

#include "mir/input/virtual_input_device.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/device.h"
#include "mir/input/input_sink.h"
#include "mir/input/event_builder.h"
#include "mir/geometry/displacement_f.h"
#include "mir/geometry/point_f.h"
#include "mir/geometry/rectangles.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;
namespace mg = mir::graphics;
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

class VirtualPointerManagerV1Global
    : public wayland::VirtualPointerManagerV1::Global
{
public:
    VirtualPointerManagerV1Global(wl_display* display, std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};

class VirtualPointerManagerV1
    : public wayland::VirtualPointerManagerV1
{
public:
    VirtualPointerManagerV1(wl_resource* resource, std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    void create_virtual_pointer(std::optional<wl_resource*> const& seat, wl_resource* id) override;
    void create_virtual_pointer_with_output(
        std::optional<wl_resource*> const& seat,
        std::optional<wl_resource*> const& output,
        wl_resource* id) override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};

class VirtualPointerV1
    : public wayland::VirtualPointerV1
{
public:
    VirtualPointerV1(
        wl_resource* resource,
        std::optional<wl_resource*> output,
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
        geometry::DisplacementF scroll_precise;
        geometry::Displacement scroll_discrete;
        bool scroll_stop_x{false}, scroll_stop_y{false};
        MirPointerButtons buttons_pressed;
    };

    geometry::Rectangle absolute_motion_area;
    MirPointerButtons buttons_pressed{0};
    geometry::PointF position;
    Pending pending{position, buttons_pressed};
};
}
}

auto mf::create_virtual_pointer_manager_v1(
    wl_display* display,
    OutputManager* output_manager,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<mw::VirtualPointerManagerV1::Global>
{
    auto ctx = std::shared_ptr<VirtualPointerV1Ctx>{new VirtualPointerV1Ctx{output_manager, device_registry}};
    return std::make_shared<VirtualPointerManagerV1Global>(display, std::move(ctx));
}

mf::VirtualPointerManagerV1Global::VirtualPointerManagerV1Global(
    wl_display* display,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : Global{display, Version<2>()},
      ctx{ctx}
{
}

void mf::VirtualPointerManagerV1Global::bind(wl_resource* new_resource)
{
    new VirtualPointerManagerV1{new_resource, ctx};
}

mf::VirtualPointerManagerV1::VirtualPointerManagerV1(
    wl_resource* resource,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerManagerV1{resource, Version<2>()},
      ctx{ctx}
{
}

void mf::VirtualPointerManagerV1::create_virtual_pointer(
    std::optional<wl_resource*> const& seat,
    wl_resource* id)
{
    (void)seat;
    new VirtualPointerV1{id, std::nullopt, ctx};
}

void mf::VirtualPointerManagerV1::create_virtual_pointer_with_output(
    std::optional<wl_resource*> const& seat,
    std::optional<wl_resource*> const& output,
    wl_resource* id)
{
    (void)seat;
    (void)output;
    new VirtualPointerV1{id, output, ctx};
}


mf::VirtualPointerV1::VirtualPointerV1(
    wl_resource* resource,
    std::optional<wl_resource*> output,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerV1{resource, Version<2>()},
      ctx{ctx},
      pointer_device{std::make_shared<mi::VirtualInputDevice>("virtual-pointer", mi::DeviceCapability::pointer)},
      output{ctx->output_manager->output_for(
          ctx->output_manager->output_id_for(output).value_or(
              mg::DisplayConfigurationOutputId{})).value_or(nullptr)}
{
    update_absolute_motion_area();
    ctx->device_registry->add_device(pointer_device);
}

mf::VirtualPointerV1::~VirtualPointerV1()
{
    ctx->device_registry->remove_device(pointer_device);
}

void mf::VirtualPointerV1::motion(uint32_t time, double dx, double dy)
{
    pending.timestamp = std::chrono::milliseconds{time};
    pending.position += geom::DisplacementF{dx, dy};
    // Leave has_absolute_motion true if we previously got a .motion_absolute request in this frame
}

void mf::VirtualPointerV1::motion_absolute(uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent)
{
    pending.timestamp = std::chrono::milliseconds{time};
    auto const local_x = geom::DeltaXF{x} * absolute_motion_area.size.width.as_value() / x_extent;
    auto const local_y = geom::DeltaYF{y} * absolute_motion_area.size.height.as_value() / y_extent;
    pending.position = geom::PointF{absolute_motion_area.top_left} + local_x + local_y;
    pending.has_absolute_motion = true;
}

void mf::VirtualPointerV1::button(uint32_t time, uint32_t button, uint32_t state)
{
    pending.timestamp = std::chrono::milliseconds{time};
    if (auto const mir_button = WlPointer::linux_button_to_mir_button(button))
    {
        switch (state)
        {
        case mw::Pointer::ButtonState::pressed:
            pending.buttons_pressed |= mir_button.value();
            break;

        case mw::Pointer::ButtonState::released:
            pending.buttons_pressed &= ~mir_button.value();
            break;

        default:
            BOOST_THROW_EXCEPTION(
                mw::ProtocolError(resource, mw::generic_error_code, "Invalid button state %d", state));
        }
    }
    else
    {
        // Since the set of allowed buttons is not clearly defined, we warn instead of throwing a protocol error
        log_warning("%s.button given unknown button %d", interface_name, button);
    }
}

void mf::VirtualPointerV1::axis(uint32_t time, uint32_t axis, double value)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mw::Pointer::Axis::horizontal_scroll:
        pending.scroll_precise.dx += geom::DeltaXF{value};
        break;

    case mw::Pointer::Axis::vertical_scroll:
        pending.scroll_precise.dy += geom::DeltaYF{value};
        break;

    default:
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::invalid_axis, "Unknown axis %d", axis));
    }
}

void mf::VirtualPointerV1::frame()
{
    pointer_device->if_started_then([&](input::InputSink* sink, input::EventBuilder* builder)
        {
            if (position != pending.position)
            {
                auto const delta = pending.position - position;
                sink->handle_input(builder->pointer_event(
                    pending.timestamp,
                    mir_pointer_action_motion,
                    buttons_pressed,
                    pending.position.x.as_value(), pending.position.y.as_value(),
                    0, 0,
                    delta.dx.as_value(), delta.dy.as_value()));
            }

            if (pending.scroll_discrete != geom::Displacement{})
            {
                sink->handle_input(builder->pointer_axis_discrete_scroll_event(
                    pending.axis_source,
                    pending.timestamp,
                    mir_pointer_action_motion,
                    buttons_pressed,
                    pending.scroll_precise.dx.as_value(), pending.scroll_precise.dy.as_value(),
                    pending.scroll_discrete.dx.as_value(), pending.scroll_discrete.dy.as_value()));
            }
            else if (pending.scroll_precise != geom::DisplacementF{})
            {
                sink->handle_input(builder->pointer_axis_event(
                    pending.axis_source,
                    pending.timestamp,
                    mir_pointer_action_motion,
                    buttons_pressed,
                    pending.position.x.as_value(), pending.position.y.as_value(),
                    pending.scroll_precise.dx.as_value(), pending.scroll_precise.dy.as_value(),
                    0, 0));
            }

            if (pending.scroll_stop_x || pending.scroll_stop_y)
            {
                sink->handle_input(builder->pointer_axis_with_stop_event(
                    pending.axis_source,
                    pending.timestamp,
                    mir_pointer_action_motion,
                    buttons_pressed,
                    pending.position.x.as_value(), pending.position.y.as_value(),
                    0, 0,
                    pending.scroll_stop_x, pending.scroll_stop_y,
                    0, 0));
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
                    0, 0, 0, 0));
            }

            if (pressed_buttons)
            {
                sink->handle_input(builder->pointer_event(
                    pending.timestamp,
                    mir_pointer_action_button_down,
                    pending.buttons_pressed,
                    0, 0, 0, 0));
            }
        });
    buttons_pressed = pending.buttons_pressed;
    position = pending.position;
    pending = Pending{position, buttons_pressed};
}

void mf::VirtualPointerV1::axis_source(uint32_t axis_source)
{
    switch (axis_source)
    {
    case mw::Pointer::AxisSource::wheel: pending.axis_source = mir_pointer_axis_source_wheel; break;
    case mw::Pointer::AxisSource::finger: pending.axis_source = mir_pointer_axis_source_finger; break;
    case mw::Pointer::AxisSource::continuous: pending.axis_source = mir_pointer_axis_source_continuous; break;
    case mw::Pointer::AxisSource::wheel_tilt: pending.axis_source = mir_pointer_axis_source_wheel_tilt; break;
    default:
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::invalid_axis_source, "Unknown axis source %d", axis_source));
    }
}

void mf::VirtualPointerV1::axis_stop(uint32_t time, uint32_t axis)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mw::Pointer::Axis::horizontal_scroll: pending.scroll_stop_x = true; break;
    case mw::Pointer::Axis::vertical_scroll: pending.scroll_stop_y = true; break;
    default:
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::invalid_axis, "Unknown axis %d", axis));
    }
}

void mf::VirtualPointerV1::axis_discrete(uint32_t time, uint32_t axis, double value, int32_t discrete)
{
    pending.timestamp = std::chrono::milliseconds{time};
    switch (axis)
    {
    case mw::Pointer::Axis::horizontal_scroll:
        pending.scroll_discrete.dx += geom::DeltaX{discrete};
        pending.scroll_precise.dx += geom::DeltaXF{value};
        break;

    case mw::Pointer::Axis::vertical_scroll:
        pending.scroll_discrete.dy += geom::DeltaY{discrete};
        pending.scroll_precise.dy += geom::DeltaYF{value};
        break;

    default:
        BOOST_THROW_EXCEPTION(
            mw::ProtocolError(resource, Error::invalid_axis, "Unknown axis %d", axis));
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
