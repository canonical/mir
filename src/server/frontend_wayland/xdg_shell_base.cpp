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

#include "xdg_shell_base.h"

#include "basic_surface_event_sink.h"
#include "wayland_utils.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include "wl_mir_window.h"

#include "mir/scene/surface_creation_parameters.h"
#include "mir/frontend/shell.h"
#include "mir/optional_value.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

class XdgSurfaceBaseEventSink : public BasicSurfaceEventSink
{
public:
    using BasicSurfaceEventSink::BasicSurfaceEventSink;

    XdgSurfaceBaseEventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink,
                            std::shared_ptr<bool> const& destroyed);

    void send_resize(geometry::Size const& new_size) const override;

    std::function<void(geometry::Size const& new_size)> notify_resize = [](auto){};

private:
    void post_configure(int serial) const;

    std::shared_ptr<bool> const destroyed;
};

}
}

// XdgSurfaceBase

//mf::XdgSurfaceBase* mf::XdgSurfaceBase::get_xdgsurface(wl_resource* surface) const
//{
//    auto* tmp = wl_resource_get_user_data(surface);
//    return static_cast<XdgSurfaceBase*>(static_cast<wayland::XdgSurface*>(tmp));
//}

mf::XdgSurfaceBase::XdgSurfaceBase(AdapterInterface const& adapter, wl_client* client, wl_resource* resource,
                                   wl_resource* parent, wl_resource* surface,
                                   std::shared_ptr<mf::Shell> const& shell, WlSeat& seat)
    : WlAbstractMirWindow{client, surface, resource, shell},
      parent{parent},
      shell{shell},
      sink{std::make_shared<XdgSurfaceBaseEventSink>(&seat, client, surface, resource, destroyed)},
      adapter{adapter}
{
    WlAbstractMirWindow::sink = sink;
}

mf::XdgSurfaceBase::~XdgSurfaceBase()
{
    WlSurface::from(surface)->set_role(null_wl_mir_window_ptr);
}

void mf::XdgSurfaceBase::destroy()
{
    wl_resource_destroy(adapter.get_resource());
}

void mf::XdgSurfaceBase::become_toplevel(uint32_t id)
{
    adapter.create_toplevel(id);
    WlSurface::from(surface)->set_role(this);
}

void mf::XdgSurfaceBase::become_popup(uint32_t id,
                                      std::experimental::optional<XdgSurfaceBase const* const> const& parent,
                                      XdgPositionerBase const& positioner)
{
    auto const session = get_session(client);

    params->type = mir_window_type_freestyle;

    if (parent)
        params->parent_id = parent.value()->surface_id;
    if (positioner.size.is_set())
        params->size = positioner.size.value();
    params->aux_rect = positioner.aux_rect;
    params->surface_placement_gravity = positioner.surface_placement_gravity;
    params->aux_rect_placement_gravity = positioner.aux_rect_placement_gravity;
    params->aux_rect_placement_offset_x = positioner.aux_rect_placement_offset_x;
    params->aux_rect_placement_offset_y = positioner.aux_rect_placement_offset_y;
    params->placement_hints = mir_placement_hints_slide_any;

    adapter.create_popup(id);
    WlSurface::from(surface)->set_role(this);
}

void mf::XdgSurfaceBase::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    WlSurface::from(surface)->buffer_offset = geom::Displacement{-x, -y};
    window_size = geom::Size{width, height};
}

void mf::XdgSurfaceBase::ack_configure(uint32_t serial)
{
    (void)serial;
    // TODO
}

void mf::XdgSurfaceBase::set_title(std::string const& title)
{
    if (surface_id.as_value())
    {
        spec().name = title;
    }
    else
    {
        params->name = title;
    }
}

void mf::XdgSurfaceBase::move()
{
    if (surface_id.as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(session, surface_id, sink->latest_timestamp(), Shell::UserRequest::move);
        }
    }
}

void mf::XdgSurfaceBase::resize(MirResizeEdge edge)
{
    if (surface_id.as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(
                session,
                surface_id,
                sink->latest_timestamp(),
                Shell::UserRequest::resize,
                edge);
        }
    }
}

void mf::XdgSurfaceBase::set_parent(optional_value<SurfaceId> parent_id)
{
    if (surface_id.as_value())
    {
        spec().parent_id = parent_id;
    }
    else
    {
        params->parent_id = parent_id;
    }
}

void mf::XdgSurfaceBase::set_max_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        if (width == 0) width = std::numeric_limits<int>::max();
        if (height == 0) height = std::numeric_limits<int>::max();

        auto& mods = spec();
        mods.max_width = geom::Width{width};
        mods.max_height = geom::Height{height};
    }
    else
    {
        if (width == 0)
        {
            if (params->max_width.is_set())
                params->max_width.consume();
        }
        else
            params->max_width = geom::Width{width};

        if (height == 0)
        {
            if (params->max_height.is_set())
                params->max_height.consume();
        }
        else
            params->max_height = geom::Height{height};
    }
}

void mf::XdgSurfaceBase::set_min_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        auto& mods = spec();
        mods.min_width = geom::Width{width};
        mods.min_height = geom::Height{height};
    }
    else
    {
        params->min_width = geom::Width{width};
        params->min_height = geom::Height{height};
    }
}

void mf::XdgSurfaceBase::set_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_maximized;
    }
    else
    {
        params->state = mir_window_state_maximized;
    }
}

void mf::XdgSurfaceBase::unset_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_restored;
    }
    else
    {
        params->state = mir_window_state_restored;
    }
}

// XdgSurfaceBaseEventSink

mf::XdgSurfaceBaseEventSink::XdgSurfaceBaseEventSink(WlSeat* seat, wl_client* client, wl_resource* target,
                                                 wl_resource* event_sink, std::shared_ptr<bool> const& destroyed)
    : BasicSurfaceEventSink(seat, client, target, event_sink),
      destroyed{destroyed}
{
    auto const serial = wl_display_next_serial(wl_client_get_display(client));
    post_configure(serial);
}

void mf::XdgSurfaceBaseEventSink::send_resize(geometry::Size const& new_size) const
{
    if (window_size != new_size)
    {
        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        notify_resize(new_size);
        post_configure(serial);
    }
}

void mf::XdgSurfaceBaseEventSink::post_configure(int serial) const
{
    seat->spawn(run_unless(destroyed, [event_sink= event_sink, serial]()
        {
            wl_resource_post_event(event_sink, 0, serial);
        }));
}
