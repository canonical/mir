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

#include <mir/log.h>

#include "wl_subcompositor.h"
#include "wl_surface.h"

#include "client.h"
#include "protocol_error.h"
#include <mir/geometry/rectangle.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;

mf::WlSubcompositor::WlSubcompositor(std::shared_ptr<wayland_rs::Client> const& client)
    : client(client)
{
}

auto mf::WlSubcompositor::get_subsurface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent)
    -> std::shared_ptr<wayland_rs::WlSubsurfaceImpl>
{
    return std::make_shared<WlSubsurface>(client, surface, parent);
}

mf::WlSubsurface::WlSubsurface(
    std::shared_ptr<wayland_rs::Client> const& client,
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface_impl, wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent_impl)
    : client{client},
      surface{WlSurface::from(&surface_impl.value())->shared_from_this()},
      parent{WlSurface::from(&parent_impl.value())->shared_from_this()},
      synchronized_{true}
{
    parent.value().add_subsurface(this);
    surface.value().set_role(this);
    surface.value().pending_invalidate_surface_data();
}

mf::WlSubsurface::~WlSubsurface()
{
    if (parent)
    {
        parent.value().remove_subsurface(this);
    }
    surface.value().clear_role();
    refresh_surface_data_now();
}

void mf::WlSubsurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                             std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                                             geometry::Displacement const& parent_offset) const
{
    if (surface.value().buffer_size())
    {
        // surface is mapped
        surface.value().populate_surface_data(buffer_streams, input_shape_accumulator, parent_offset);
    }
}

auto mf::WlSubsurface::total_offset() const -> geom::Displacement
{
    return parent ?
        parent.value().offset() :
        geom::Displacement{};
}

auto mf::WlSubsurface::synchronized() const -> bool
{
    bool const parent_synchronized = parent && parent.value().synchronized();
    return synchronized_ || parent_synchronized;
}

auto mf::WlSubsurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    return parent ?
        parent.value().scene_surface() :
        std::nullopt;
}

void mf::WlSubsurface::parent_has_committed()
{
    if (cached_state && synchronized())
    {
        surface.value().commit(cached_state.value());
        cached_state = std::nullopt;
    }
}

auto mf::WlSubsurface::subsurface_at(geom::Point point) -> std::optional<WlSurface*>
{
    return surface.value().subsurface_at(point);
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    surface.value().set_pending_offset(geom::Displacement{x, y});
}

void mf::WlSubsurface::place_above(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling)
{
    if (!parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    WlSurface* sibling_surface = WlSurface::from(&sibling.value());

    if (sibling_surface == &surface.value())
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_above: sibling cannot be the subsurface itself"));
    }

    if (sibling_surface != &parent.value() && !parent.value().has_subsurface_with_surface(sibling_surface))
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_above: sibling must be the parent or a sibling subsurface"));
    }

    parent.value().reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::above);
}

void mf::WlSubsurface::place_below(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling)
{
    if (!parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    WlSurface* sibling_surface = WlSurface::from(&sibling.value());

    if (sibling_surface == &surface.value())
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_below: sibling cannot be the subsurface itself"));
    }

    if (sibling_surface != &parent.value() && !parent.value().has_subsurface_with_surface(sibling_surface))
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_below: sibling must be the parent or a sibling subsurface"));
    }

    parent.value().reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::below);
}

void mf::WlSubsurface::set_sync()
{
    synchronized_ = true;
}

void mf::WlSubsurface::set_desync()
{
    synchronized_ = false;
}

void mf::WlSubsurface::refresh_surface_data_now()
{
    if (parent)
    {
        parent.value().refresh_surface_data_now();
    }
}

void mf::WlSubsurface::commit(WlSurfaceState const& state)
{
    if (!cached_state)
        cached_state = WlSurfaceState();

    cached_state.value().update_from(state);

    if (cached_state.value().buffer)
    {
        bool const currently_mapped = static_cast<bool>(surface.value().buffer_size());
        bool const pending_mapped = cached_state.value().buffer.value();
        if (currently_mapped != pending_mapped)
        {
            cached_state.value().invalidate_surface_data();
        }
    }

    if (synchronized())
    {
        if (cached_state.value().surface_data_needs_refresh() && parent)
        {
            parent.value().pending_invalidate_surface_data();
        }
    }
    else
    {
        surface.value().commit(cached_state.value());
        if (cached_state.value().surface_data_needs_refresh())
            refresh_surface_data_now();
        cached_state = std::nullopt;
    }
}

void mf::WlSubsurface::surface_destroyed()
{
    if (!client->is_being_destroyed())
    {
        // "When a client wants to destroy a wl_surface, they must destroy this 'role object' wl_surface"
        BOOST_THROW_EXCEPTION(std::runtime_error{
            "wl_surface@" + std::to_string(surface.value().object_id()) +
            " destroyed before it's associated wl_subsurface@" + std::to_string(object_id())});
    }
    else
    {
        // If the client has been destroyed, everything is getting cleaned up in an arbitrary order. Delete this so our
        // derived class doesn't end up using the now-defunct surface.
        delete this;
    }
}
