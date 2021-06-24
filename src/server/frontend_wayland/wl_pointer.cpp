/*
 * Copyright © 2018-2021 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
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
        : buffer{mrs::as_read_mappable_buffer(std::move(buffer))},
          mapping{this->buffer->map_readable()},
          hotspot_(hotspot)
    {
    }

    auto as_argb_8888() const -> void const* override
    {
        return mapping->data();
    }

    auto size() const -> geom::Size override
    {
        return mapping->size();
    }

    auto hotspot() const -> geom::Displacement override
    {
        return hotspot_;
    }

private:
    std::shared_ptr<mrs::ReadMappableBuffer> const buffer;
    std::unique_ptr<mrs::Mapping<unsigned char const>> const mapping;
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

auto timestamp_of(MirPointerEvent const* event) -> uint32_t
{
    return mir_input_event_get_wayland_timestamp(mir_pointer_event_input_event(event));
}
}

struct mf::WlPointer::Cursor
{
    virtual void apply_to(WlSurface* surface) = 0;
    virtual void set_hotspot(geom::Displacement const& new_hotspot) = 0;
    virtual auto cursor_surface() const -> std::experimental::optional<WlSurface*> = 0;

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
    auto cursor_surface() const -> std::experimental::optional<mf::WlSurface*> override { return {}; };
};
}

mf::WlPointer::WlPointer(wl_resource* new_resource)
    : Pointer(new_resource, Version<6>()),
      display{wl_client_get_display(client)},
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

void mir::frontend::WlPointer::event(MirPointerEvent const* event, WlSurface& root_surface)
{
    switch(mir_pointer_event_action(event))
    {
        case mir_pointer_action_button_down:
        case mir_pointer_action_button_up:
            buttons(event);
            break;
        case mir_pointer_action_enter:
            leave(event); // If we're currently on a surface, leave it
            enter_or_motion(event, root_surface);
            break;
        case mir_pointer_action_leave:
            leave(event);
            break;
        case mir_pointer_action_motion:
            enter_or_motion(event, root_surface);
            relative_motion(event);
            axis(event);
            break;
        case mir_pointer_actions:
            break;
    }

    maybe_frame();
}

void mf::WlPointer::leave(std::optional<MirPointerEvent const*> event)
{
    (void)event;
    if (!surface_under_cursor)
        return;
    surface_under_cursor.value().remove_destroy_listener(destroy_listener_id);
    auto const serial = wl_display_next_serial(display);
    send_leave_event(
        serial,
        surface_under_cursor.value().raw_resource());
    current_position = std::experimental::nullopt;
    // Don't clear current_buttons, their state can survive leaving and entering surfaces (note we currently have logic
    // to prevent changing surfaces while buttons are pressed, we wouln't need to clear current_buttons regardless)
    needs_frame = true;
    surface_under_cursor = {};
    destroy_listener_id = {};
}

void mf::WlPointer::buttons(MirPointerEvent const* event)
{
    MirPointerButtons const event_buttons = mir_pointer_event_buttons(event);

    for (auto const& mapping : button_mapping)
    {
        // Check if the state of this button changed
        if (mapping.first & (event_buttons ^ current_buttons))
        {
            bool const pressed = (mapping.first & event_buttons);
            auto const state = pressed ? ButtonState::pressed : ButtonState::released;
            auto const serial = wl_display_next_serial(display);
            send_button_event(serial, timestamp_of(event), mapping.second, state);
            needs_frame = true;
        }
    }

    current_buttons = event_buttons;
}

void mf::WlPointer::axis(MirPointerEvent const* event)
{
    auto const h_scroll = mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll);
    auto const v_scroll = mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll);

    if (h_scroll)
    {
        send_axis_event(
            timestamp_of(event),
            Axis::horizontal_scroll,
            h_scroll);
        needs_frame = true;
    }

    if (v_scroll)
    {
        send_axis_event(
            timestamp_of(event),
            Axis::vertical_scroll,
            v_scroll);
        needs_frame = true;
    }
}

void mf::WlPointer::enter_or_motion(MirPointerEvent const* event, WlSurface& root_surface)
{
    auto const root_position = std::make_pair(
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y));

    WlSurface* target_surface;
    if (current_buttons != 0 && surface_under_cursor)
    {
        // If there are pressed buttons, we let the pointer move outside the current surface without leaving it
        target_surface = &surface_under_cursor.value();
    }
    else
    {
        // Else choose whatever subsurface we are over top of
        geom::Point root_point{root_position.first, root_position.second};
        target_surface = root_surface.subsurface_at(root_point).value_or(&root_surface);
    }

    auto const offset = target_surface->total_offset();
    auto const position_on_target = std::make_pair(
        root_position.first - offset.dx.as_int(),
        root_position.second - offset.dy.as_int());

    if (!surface_under_cursor || &surface_under_cursor.value() != target_surface)
    {
        // We need to switch surfaces
        leave(event); // If we're currently on a surface, leave it
        auto const serial = wl_display_next_serial(display);
        cursor->apply_to(target_surface);
        send_enter_event(
            serial,
            target_surface->raw_resource(),
            position_on_target.first,
            position_on_target.second);
        current_position = position_on_target;
        needs_frame = true;
        destroy_listener_id = target_surface->add_destroy_listener(
            [this]()
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
                position_on_target.first,
                position_on_target.second);
            current_position = position_on_target;
            needs_frame = true;
        }
    }
}

void mf::WlPointer::relative_motion(MirPointerEvent const* event)
{
    if (!relative_pointer)
    {
        return;
    }
    auto const motion = std::make_pair(
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y));
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
    if (needs_frame && version_supports_frame())
    {
        send_frame_event();
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
    auto cursor_surface() const -> std::experimental::optional<mf::WlSurface*> override;

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
    auto cursor_surface() const -> std::experimental::optional<mf::WlSurface*> override { return {}; };

private:
    CursorSurfaceRole surface_role;
};
}

void mf::WlPointer::set_cursor(
    uint32_t serial,
    std::experimental::optional<wl_resource*> const& surface,
    int32_t hotspot_x, int32_t hotspot_y)
{
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

    (void)serial;
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

auto WlSurfaceCursor::cursor_surface() const -> std::experimental::optional<mf::WlSurface*>
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
