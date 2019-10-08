/*
 * Copyright © 2018 Canonical Ltd.
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

#include "mir/executor.h"
#include "mir/frontend/wayland.h"
#include "mir/scene/surface.h"
#include "mir/frontend/buffer_stream.h"
#include "mir/geometry/displacement.h"

#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

struct mf::WlPointer::Cursor
{
    virtual void apply_to(WlSurface* surface) = 0;
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
};
}

mf::WlPointer::WlPointer(
    wl_resource* new_resource,
    std::function<void(WlPointer*)> const& on_destroy)
    : Pointer(new_resource, Version<6>()),
      display{wl_client_get_display(client)},
      on_destroy{on_destroy},
      cursor{std::make_unique<NullCursor>()}
{
}

mf::WlPointer::~WlPointer()
{
    if (surface_under_cursor)
        surface_under_cursor.value()->remove_destroy_listener(this);
    on_destroy(this);
}

void mf::WlPointer::enter(WlSurface* parent_surface, geom::Point const& position_on_parent)
{
    auto const serial = wl_display_next_serial(display);
    auto const final = parent_surface->transform_point(position_on_parent);

    cursor->apply_to(final.surface);
    send_enter_event(
        serial,
        final.surface->raw_resource(),
        final.position.x.as_int(),
        final.position.y.as_int());
    can_send_frame = true;
    final.surface->add_destroy_listener(
        this,
        [this]()
        {
            leave();
        });
    surface_under_cursor = final.surface;
}

void mf::WlPointer::leave()
{
    if (!surface_under_cursor)
        return;
    auto const surface = surface_under_cursor.value();
    surface->remove_destroy_listener(this);
    if (auto mir_surface = surface->scene_surface())
    {
        // TODO [purge-mirclient] We should clean up once we've got no mirclient to support:
        // Called for side-effect set_cursor_image() implicitly detatches the cursor stream
        mir_surface.value()->set_cursor_image({});
    }
    auto const serial = wl_display_next_serial(display);
    send_leave_event(serial, surface->raw_resource());
    can_send_frame = true;
    surface_under_cursor = std::experimental::nullopt;
}

void mf::WlPointer::button(std::chrono::milliseconds const& ms, uint32_t button, bool pressed)
{
    auto const serial = wl_display_next_serial(display);
    auto const state = pressed ? ButtonState::pressed : ButtonState::released;

    send_button_event(serial, ms.count(), button, state);
    can_send_frame = true;
}

void mf::WlPointer::motion(
    std::chrono::milliseconds const& ms,
    WlSurface* parent_surface,
    geometry::Point const& position_on_parent)
{
    auto final = parent_surface->transform_point(position_on_parent);

    if (surface_under_cursor && final.surface == surface_under_cursor.value())
    {
        send_motion_event(
            ms.count(),
            final.position.x.as_int(),
            final.position.y.as_int());
        can_send_frame = true;
    }
    else
    {
        leave();
        enter(final.surface, final.position);
    }
}

void mf::WlPointer::axis(std::chrono::milliseconds const& ms, geometry::Displacement const& scroll)
{
    if (scroll.dx != geom::DeltaX{})
    {
        send_axis_event(
            ms.count(),
            Axis::horizontal_scroll,
            scroll.dx.as_int());
        can_send_frame = true;
    }

    if (scroll.dy != geom::DeltaY{})
    {
        send_axis_event(
            ms.count(),
            Axis::vertical_scroll,
            scroll.dy.as_int());
        can_send_frame = true;
    }
}

void mf::WlPointer::frame()
{
    if (can_send_frame && version_supports_frame())
        send_frame_event();
    can_send_frame = false;
}

namespace
{
struct WlStreamCursor : mf::WlPointer::Cursor
{
    WlStreamCursor(
        std::shared_ptr<mf::BufferStream> const& stream,
        geom::Displacement hotspot);

    void apply_to(mf::WlSurface* surface) override;

    std::shared_ptr<mf::BufferStream> const stream;
    geom::Displacement const hotspot;
};

struct WlHiddenCursor : mf::WlPointer::Cursor
{
    WlHiddenCursor();
    void apply_to(mf::WlSurface* surface) override;
};
}

void mf::WlPointer::set_cursor(
    uint32_t serial,
    std::experimental::optional<wl_resource*> const& surface,
    int32_t hotspot_x, int32_t hotspot_y)
{
    if (surface)
    {
        auto const cursor_stream = WlSurface::from(*surface)->stream;
        geom::Displacement const cursor_hotspot{hotspot_x, hotspot_y};

        cursor = std::make_unique<WlStreamCursor>(cursor_stream, cursor_hotspot);
    }
    else
    {
        cursor = std::make_unique<WlHiddenCursor>();
    }

    if (surface_under_cursor)
        cursor->apply_to(surface_under_cursor.value());

    (void)serial;
}

void mf::WlPointer::release()
{
    destroy_wayland_object();
}

WlStreamCursor::WlStreamCursor(
    std::shared_ptr<mf::BufferStream> const& stream,
    geom::Displacement hotspot) :
    stream{stream},
    hotspot{hotspot}
{
}

void WlStreamCursor::apply_to(mf::WlSurface* surface)
{
    if (auto scene_surface = surface->scene_surface())
    {
        scene_surface.value()->set_cursor_stream(stream, hotspot);
    }
}

WlHiddenCursor::WlHiddenCursor()
{
}

void WlHiddenCursor::apply_to(mf::WlSurface* surface)
{
    if (auto scene_surface = surface->scene_surface())
    {
        scene_surface.value()->set_cursor_image({});
    }
}
