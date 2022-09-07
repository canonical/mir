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

#include "wl_pointer.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_seat.h"
#include "relative-pointer-unstable-v1_wrapper.h"

#include "mir/log.h"
#include "mir/executor.h"
#include "mir/frontend/wayland.h"
#include "mir/scene/surface.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/buffer.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/events/pointer_event.h"
#include "mir/wayland/client.h"

#include <linux/input-event-codes.h>
#include <boost/throw_exception.hpp>
#include <string.h> // memcpy

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mrs = mir::renderer::software;

namespace
{
class BufferCursorImage : public mg::CursorImage
{
public:
    BufferCursorImage(std::shared_ptr<mg::Buffer> buffer, geom::Displacement const& hotspot)
        : BufferCursorImage(buffer_to_mapping_if_possible(buffer), hotspot)
    {
    }

    auto as_argb_8888() const -> void const* override
    {
        return data.get();
    }

    auto size() const -> geom::Size override
    {
        return size_;
    }

    auto hotspot() const -> geom::Displacement override
    {
        return hotspot_;
    }

private:
    BufferCursorImage(std::unique_ptr<mrs::Mapping<unsigned char const>> mapping, geom::Displacement const& hotspot)
        : size_{mapping->size()},
          data{std::make_unique<unsigned char[]>(mapping->len())},
          hotspot_{hotspot}
    {
        ::memcpy(data.get(), mapping->data(), mapping->len());
    }

    static auto buffer_to_mapping_if_possible(std::shared_ptr<mg::Buffer> const& buffer)
        -> std::unique_ptr<mrs::Mapping<unsigned char const>>
    {
        auto const mappable_buffer = mrs::as_read_mappable_buffer(buffer);
        if (!mappable_buffer)
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error{
                    "Attempt to create cursor from non-CPU-readable buffer. Rendering will be incomplete"});
        }
        return mappable_buffer->map_readable();
    }

    geom::Size const size_;
    std::unique_ptr<unsigned char[]> const data;
    geom::Displacement const hotspot_;
};

static auto const button_mapping = {
    std::make_pair(mir_pointer_button_primary, BTN_LEFT),
    std::make_pair(mir_pointer_button_secondary, BTN_RIGHT),
    std::make_pair(mir_pointer_button_tertiary, BTN_MIDDLE),
    std::make_pair(mir_pointer_button_back, BTN_BACK),
    std::make_pair(mir_pointer_button_forward, BTN_FORWARD),
    std::make_pair(mir_pointer_button_side, BTN_SIDE),
    std::make_pair(mir_pointer_button_task, BTN_TASK),
    std::make_pair(mir_pointer_button_extra, BTN_EXTRA)
};

auto timestamp_of(std::shared_ptr<MirPointerEvent const> const& event) -> uint32_t
{
    return mir_input_event_get_wayland_timestamp(mir_pointer_event_input_event(event.get()));
}

auto wayland_axis_source(MirPointerAxisSource mir_source) -> std::optional<uint32_t>
{
    switch (mir_source)
    {
    case mir_pointer_axis_source_none:
        return std::nullopt;

    case mir_pointer_axis_source_wheel:
        return mw::Pointer::AxisSource::wheel;

    case mir_pointer_axis_source_finger:
        return mw::Pointer::AxisSource::finger;

    case mir_pointer_axis_source_continuous:
        return mw::Pointer::AxisSource::continuous;

    case mir_pointer_axis_source_wheel_tilt:
        return mw::Pointer::AxisSource::wheel_tilt;
    }

    mir::fatal_error("Invalid MirPointerAxisSource %d", mir_source);
    return std::nullopt;
}
}

struct mf::WlPointer::Cursor
{
    virtual void apply_to(WlSurface* surface) = 0;
    virtual void set_hotspot(geom::Displacement const& new_hotspot) = 0;
    virtual auto cursor_surface() const -> std::optional<WlSurface*> = 0;

    virtual ~Cursor() = default;
    Cursor() = default;

    Cursor(Cursor const&) = delete;
    Cursor& operator=(Cursor const&) = delete;
};

namespace
{
struct NullCursor : mf::WlPointer::Cursor
{
    void apply_to(mf::WlSurface*) override {}
    void set_hotspot(geom::Displacement const&) override {};
    auto cursor_surface() const -> std::optional<mf::WlSurface*> override { return {}; };
};
}

auto mf::WlPointer::linux_button_to_mir_button(int linux_button) -> std::optional<MirPointerButtons>
{
    for (auto const& mapping : button_mapping)
    {
        if (mapping.second == linux_button)
        {
            return mapping.first;
        }
    }
    return std::nullopt;
}

mf::WlPointer::WlPointer(wl_resource* new_resource)
    : Pointer(new_resource, Version<8>()),
      cursor{std::make_unique<NullCursor>()}
{
}

mf::WlPointer::~WlPointer()
{
    if (surface_under_cursor)
        surface_under_cursor.value().remove_destroy_listener(destroy_listener_id);
}

void mir::frontend::WlPointer::set_relative_pointer(mir::wayland::RelativePointerV1* relative_ptr)
{
    relative_pointer = make_weak(relative_ptr);
}

void mir::frontend::WlPointer::event(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface)
{
    switch(mir_pointer_event_action(event.get()))
    {
        case mir_pointer_action_button_down:
        case mir_pointer_action_button_up:
            enter_or_motion(event, root_surface);
            buttons(event);
            break;
        case mir_pointer_action_enter:
            enter_or_motion(event, root_surface);
            axes(event);
            break;
        case mir_pointer_action_leave:
            leave(event);
            break;
        case mir_pointer_action_motion:
            enter_or_motion(event, root_surface);
            relative_motion(event);
            axes(event);
            break;
        case mir_pointer_actions:
            break;
    }

    maybe_frame();
}

void mf::WlPointer::leave(std::optional<std::shared_ptr<MirPointerEvent const>> const& event)
{
    if (!surface_under_cursor)
        return;
    surface_under_cursor.value().remove_destroy_listener(destroy_listener_id);
    auto const serial = client->next_serial(event.value_or(nullptr));
    send_leave_event(
        serial,
        surface_under_cursor.value().raw_resource());
    current_position = std::nullopt;
    // Don't clear current_buttons, their state can survive leaving and entering surfaces (note we currently have logic
    // to prevent changing surfaces while buttons are pressed, we wouldn't need to clear current_buttons regardless)
    needs_frame = true;
    surface_under_cursor = {};
    destroy_listener_id = {};
}

void mf::WlPointer::buttons(std::shared_ptr<MirPointerEvent const> const& event)
{
    MirPointerButtons const event_buttons = mir_pointer_event_buttons(event.get());

    for (auto const& mapping : button_mapping)
    {
        // Check if the state of this button changed
        if (mapping.first & (event_buttons ^ current_buttons))
        {
            bool const pressed = (mapping.first & event_buttons);
            auto const state = pressed ? ButtonState::pressed : ButtonState::released;
            auto const serial = client->next_serial(event);;
            send_button_event(serial, timestamp_of(event), mapping.second, state);
            needs_frame = true;
        }
    }

    current_buttons = event_buttons;
}

template<typename Tag>
auto mf::WlPointer::axis(
    std::shared_ptr<MirPointerEvent const> const& event,
    events::ScrollAxis<Tag> axis,
    uint32_t wayland_axis) -> bool
{
    bool event_sent = false;

    if (axis.value120.as_value() && version_supports_axis_value120())
    {
        send_axis_value120_event(wayland_axis, axis.value120.as_value());
        event_sent = true;
    }

    // Only send discrete on versions pre-value120
    if (axis.discrete.as_value() && version_supports_axis_discrete() && !version_supports_axis_value120())
    {
        send_axis_discrete_event(wayland_axis, axis.discrete.as_value());
        event_sent = true;
    }

    // If we sent discrete or value120 scroll events, we're required to also send a non-value120 event on
    // the same axis, evin if the value is 0 (likely because axis is the event that carries the timestamp)
    if (axis.precise.as_value() || event_sent)
    {
        send_axis_event(timestamp_of(event), wayland_axis, axis.precise.as_value());
        event_sent = true;
    }

    if (axis.stop && version_supports_axis_stop())
    {
        send_axis_stop_event(timestamp_of(event), wayland_axis);
        event_sent = true;
    }

    return event_sent;
}

void mf::WlPointer::axes(std::shared_ptr<MirPointerEvent const> const& event)
{
    bool axis_event_sent = false;

    axis_event_sent |= axis(event, event->h_scroll(), Axis::horizontal_scroll);
    axis_event_sent |= axis(event, event->v_scroll(), Axis::vertical_scroll);
    needs_frame |= axis_event_sent;

    // Don't send an axis source unless we have one and we're also sending some sort of axis event.
    auto const axis_source = wayland_axis_source(event->axis_source());
    if (axis_source && axis_event_sent && version_supports_axis_source())
    {
        send_axis_source_event(axis_source.value());
        needs_frame = true;
    }
}

void mf::WlPointer::enter_or_motion(std::shared_ptr<MirPointerEvent const> const& event, WlSurface& root_surface)
{
    if (!event->local_position())
    {
        return;
    }

    auto const root_position = event->local_position().value();

    WlSurface* target_surface;
    if (current_buttons != 0 && surface_under_cursor)
    {
        // If there are pressed buttons, we let the pointer move outside the current surface without leaving it
        target_surface = &surface_under_cursor.value();
    }
    else
    {
        // Else choose whatever subsurface we are over top of
        geom::Point root_point{root_position};
        target_surface = root_surface.subsurface_at(root_point).value_or(&root_surface);
    }

    auto const position_on_target = root_position - geom::DisplacementF{target_surface->total_offset()};

    if (!surface_under_cursor || &surface_under_cursor.value() != target_surface)
    {
        // We need to switch surfaces
        leave(event); // If we're currently on a surface, leave it
        enter_serial = client->next_serial(event);
        cursor->apply_to(target_surface);
        send_enter_event(
            enter_serial.value(),
            target_surface->raw_resource(),
            position_on_target.x.as_value(),
            position_on_target.y.as_value());
        current_position = position_on_target;
        needs_frame = true;
        destroy_listener_id = target_surface->add_destroy_listener([this]()
            {
                leave(std::nullopt);
                maybe_frame();
            });
        surface_under_cursor = mw::make_weak(target_surface);
    }
    else if (position_on_target != current_position)
    {
        switch (target_surface->confine_pointer_state())
        {
        case mir_pointer_locked_oneshot:
        case mir_pointer_locked_persistent:
            break;

        default:
            send_motion_event(
                timestamp_of(event),
                position_on_target.x.as_value(),
                position_on_target.y.as_value());
            current_position = position_on_target;
            needs_frame = true;
        }
    }
}

void mf::WlPointer::relative_motion(std::shared_ptr<MirPointerEvent const> const& event)
{
    if (!relative_pointer)
    {
        return;
    }
    auto const motion = std::make_pair(
        mir_pointer_event_axis_value(event.get(), mir_pointer_axis_relative_x),
        mir_pointer_event_axis_value(event.get(), mir_pointer_axis_relative_y));
    if (motion.first || motion.second)
    {
        auto const timestamp = timestamp_of(event);
        relative_pointer.value().send_relative_motion_event(
            timestamp, timestamp,
            motion.first, motion.second,
            motion.first, motion.second);
        needs_frame = true;
    }
}

void mf::WlPointer::maybe_frame()
{
    if (needs_frame)
    {
        send_frame_event_if_supported();
    }
    needs_frame = false;
}

namespace
{
struct CursorSurfaceRole : mf::NullWlSurfaceRole
{
    mw::Weak<mf::WlSurface> const surface;
    mf::CommitHandler* const commit_handler;
    explicit CursorSurfaceRole(mf::WlSurface* surface, mf::CommitHandler* commit_handler) :
        NullWlSurfaceRole(surface),
        surface{surface},
        commit_handler{commit_handler} {}

    ~CursorSurfaceRole()
    {
        if (surface)
        {
            surface.value().clear_role();
        }
    }

    void commit(mir::frontend::WlSurfaceState const& state) override
    {
        NullWlSurfaceRole::commit(state);
        commit_handler->on_commit(&surface.value());
    }
};

struct WlSurfaceCursor : mf::WlPointer::Cursor
{
    WlSurfaceCursor(
        mf::WlSurface* surface,
        geom::Displacement hotspot,
        mf::CommitHandler* commit_handler);
    ~WlSurfaceCursor();

    void apply_to(mf::WlSurface* surface) override;
    void set_hotspot(geom::Displacement const& new_hotspot) override;
    auto cursor_surface() const -> std::optional<mf::WlSurface*> override;

private:
    void apply_latest_buffer();

    mw::Weak<mf::WlSurface> const surface;
    std::shared_ptr<mc::BufferStream> const stream;
    CursorSurfaceRole surface_role;

    std::weak_ptr<ms::Surface> surface_under_cursor;
    geom::Displacement hotspot;
};

struct WlHiddenCursor : mf::WlPointer::Cursor
{
    WlHiddenCursor(
        mf::WlSurface* surface,
        mf::CommitHandler* commit_handler);
    void apply_to(mf::WlSurface* surface) override;
    void set_hotspot(geom::Displacement const&) override {};
    auto cursor_surface() const -> std::optional<mf::WlSurface*> override { return {}; };

private:
    CursorSurfaceRole surface_role;
};
}

void mf::WlPointer::set_cursor(
    uint32_t serial,
    std::optional<wl_resource*> const& surface,
    int32_t hotspot_x, int32_t hotspot_y)
{
    if (!enter_serial || serial != enter_serial.value())
    {
        return;
    }

    // We need an explicit conversion before calling make_unique
    // (the compiler should elide this variable)
    CommitHandler* const commit_handler = this;

    if (surface)
    {
        auto const wl_surface = WlSurface::from(*surface);
        cursor_hotspot = {hotspot_x, hotspot_y};
        if (!cursor->cursor_surface() || wl_surface != *cursor->cursor_surface())
        {
            cursor.reset(); // clean up old cursor before creating new one
            cursor = std::make_unique<WlSurfaceCursor>(wl_surface, cursor_hotspot, commit_handler);
            if (surface_under_cursor)
                cursor->apply_to(&surface_under_cursor.value());
        }
        // If surface is unchanged hotspot will be applied on next commit
    }
    else
    {
        cursor = std::make_unique<WlHiddenCursor>(nullptr, commit_handler);
        if (surface_under_cursor)
            cursor->apply_to(&surface_under_cursor.value());
    }
}

void mf::WlPointer::on_commit(WlSurface* surface)
{
    // We need an explicit conversion before calling make_unique
    // (the compiler should elide this variable)
    CommitHandler* const commit_handler = this;

    if (!surface->buffer_size())
    {
        // No buffer: We should be unmapping the cursor

        cursor = std::make_unique<WlHiddenCursor>(surface, commit_handler);
        if (surface_under_cursor)
            cursor->apply_to(&surface_under_cursor.value());
    }
    else
    {
        if (cursor->cursor_surface() && surface == *cursor->cursor_surface())
        {
            cursor->set_hotspot(cursor_hotspot);
        }
        else
        {
            cursor.reset(); // clean up old cursor before creating new one
            cursor = std::make_unique<WlSurfaceCursor>(surface, cursor_hotspot, commit_handler);
            if (surface_under_cursor)
                cursor->apply_to(&surface_under_cursor.value());
        }
    }
}

WlSurfaceCursor::WlSurfaceCursor(mf::WlSurface* surface, geom::Displacement hotspot, mf::CommitHandler* commit_handler)
    : surface{surface},
      stream{surface->stream},
      surface_role{surface, commit_handler},
      hotspot{hotspot}
{
    surface->set_role(&surface_role);

    stream->set_frame_posted_callback(
        [this](auto)
        {
            this->apply_latest_buffer();
        });
}

WlSurfaceCursor::~WlSurfaceCursor()
{
    if (surface)
    {
        surface.value().clear_role();
    }
    stream->set_frame_posted_callback([](auto){});
}

void WlSurfaceCursor::apply_to(mf::WlSurface* surface)
{
    auto const scene_surface = surface->scene_surface();

    if (scene_surface)
    {
        surface_under_cursor = *scene_surface;
        apply_latest_buffer();
    }
    else
    {
        surface_under_cursor.reset();
    }
}

void WlSurfaceCursor::set_hotspot(geom::Displacement const& new_hotspot)
{
    hotspot = new_hotspot;
    apply_latest_buffer();
}

auto WlSurfaceCursor::cursor_surface() const -> std::optional<mf::WlSurface*>
{
    if (surface)
    {
        return &surface.value();
    }
    else
    {
        return {};
    }
}

void WlSurfaceCursor::apply_latest_buffer()
{
    if (auto const surface = surface_under_cursor.lock())
    {
        if (stream->has_submitted_buffer())
        {
            auto const cursor_image = std::make_shared<BufferCursorImage>(
                stream->lock_compositor_buffer(this),
                hotspot);
            surface->set_cursor_image(cursor_image);
        }
        else
        {
            surface->set_cursor_image(nullptr);
        }
    }
}

WlHiddenCursor::WlHiddenCursor(mf::WlSurface* surface, mf::CommitHandler* commit_handler) :
    surface_role{surface, std::move(commit_handler)}
{
}

void WlHiddenCursor::apply_to(mf::WlSurface* surface)
{
    if (auto scene_surface = surface->scene_surface())
    {
        scene_surface.value()->set_cursor_image({});
    }
}
