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
    auto const target = parent_surface->transform_point(position_on_parent);
    enter_internal(target.surface, target.position);
}

void mf::WlPointer::leave()
{
    if (!surface_under_cursor)
        return;
    surface_under_cursor.value()->remove_destroy_listener(this);
    auto const serial = wl_display_next_serial(display);
    send_leave_event(
        serial,
        surface_under_cursor.value()->raw_resource());
    can_send_frame = true;
    surface_under_cursor = std::experimental::nullopt;
}

void mf::WlPointer::button(std::chrono::milliseconds const& ms, uint32_t button, bool pressed)
{
    if (pressed)
    {
        if (!pressed_buttons.insert(button).second)
        {
            log_warning(
                "Got pressed event for wl_pointer@%d button %d when already down",
                wl_resource_get_id(resource),
                button);
            return;
        }
    }
    else
    {
        if (!pressed_buttons.erase(button))
        {
            log_warning(
                "Got unpressed event for wl_pointer@%d button %d when already up",
                wl_resource_get_id(resource),
                button);
            return;
        }
    }

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
        enter_internal(final.surface, position_on_parent);
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

void mf::WlPointer::enter_internal(WlSurface* surface, geometry::Point const& position)
{
    auto const serial = wl_display_next_serial(display);
    cursor->apply_to(surface);
    send_enter_event(
        serial,
        surface->raw_resource(),
        position.x.as_int(),
        position.y.as_int());
    can_send_frame = true;
    surface->add_destroy_listener(
        this,
        [this]()
        {
            leave();
        });
    surface_under_cursor = surface;
}

namespace
{
struct WlSurfaceCursor : mf::WlPointer::Cursor
{
    WlSurfaceCursor(
        mf::WlSurface* surface,
        geom::Displacement hotspot);
    ~WlSurfaceCursor();

    void apply_to(mf::WlSurface* surface) override;
    void set_hotspot(geom::Displacement const& new_hotspot) override;
    auto cursor_surface() const -> std::experimental::optional<mf::WlSurface*> override;

private:
    void apply_latest_buffer();

    mf::WlSurface* const surface;
    // If surface_destroyed is true, surface should not be used
    std::shared_ptr<bool> surface_destroyed;
    std::shared_ptr<mc::BufferStream> const stream;
    mf::NullWlSurfaceRole surface_role; // Used only to assert unique ownership

    std::weak_ptr<ms::Surface> surface_under_cursor;
    geom::Displacement hotspot;
};

struct WlHiddenCursor : mf::WlPointer::Cursor
{
    void apply_to(mf::WlSurface* surface) override;
    void set_hotspot(geom::Displacement const&) override {};
    auto cursor_surface() const -> std::experimental::optional<mf::WlSurface*> override { return {}; };
};
}

void mf::WlPointer::set_cursor(
    uint32_t serial,
    std::experimental::optional<wl_resource*> const& surface,
    int32_t hotspot_x, int32_t hotspot_y)
{
    if (surface)
    {
        auto const wl_surface = WlSurface::from(*surface);
        geom::Displacement const cursor_hotspot{hotspot_x, hotspot_y};
        if (wl_surface == cursor->cursor_surface())
        {
            cursor->set_hotspot(cursor_hotspot);
        }
        else
        {
            cursor.reset(); // clean up old cursor before creating new one
            cursor = std::make_unique<WlSurfaceCursor>(wl_surface, cursor_hotspot);
            if (surface_under_cursor)
                cursor->apply_to(surface_under_cursor.value());
        }
    }
    else
    {
        cursor = std::make_unique<WlHiddenCursor>();
        if (surface_under_cursor)
            cursor->apply_to(surface_under_cursor.value());
    }

    (void)serial;
}

void mf::WlPointer::release()
{
    destroy_wayland_object();
}

WlSurfaceCursor::WlSurfaceCursor(mf::WlSurface* surface, geom::Displacement hotspot)
    : surface{surface},
      surface_destroyed{surface->destroyed_flag()},
      stream{surface->stream},
      surface_role{surface},
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
    if (!*surface_destroyed)
        surface->clear_role();
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
    if (!*surface_destroyed)
        return surface;
    else
        return {};
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

void WlHiddenCursor::apply_to(mf::WlSurface* surface)
{
    if (auto scene_surface = surface->scene_surface())
    {
        scene_surface.value()->set_cursor_image({});
    }
}
