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
#include "protocol_error.h"

#include <mir/geometry/rectangle.h>
#include <boost/throw_exception.hpp>

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_rs/wayland_rs_cpp/include/protocol_error.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;

mf::WlSubcompositor::WlSubcompositor(rust::Box<wayland_rs::WaylandClient> client)
    : WlSubcompositorImpl(std::move(client))
{
}

auto mf::WlSubcompositor::get_subsurface(
    std::shared_ptr<wayland_rs::WlSurfaceImpl> const& surface,
    std::shared_ptr<wayland_rs::WlSurfaceImpl> const& parent)
    -> std::shared_ptr<wayland_rs::WlSubsurfaceImpl>
{
    return std::make_shared<WlSubsurface>(client->clone_box(), surface, parent);
}

mf::WlSubsurface::WlSubsurface(
    rust::Box<wayland_rs::WaylandClient> client,
    std::shared_ptr<wayland_rs::WlSurfaceImpl> const& surface,
    std::shared_ptr<wayland_rs::WlSurfaceImpl> const& parent)
    : wayland_rs::WlSubsurfaceImpl(std::move(client)),
      surface{surface},
      parent{parent},
      synchronized_{true}
{
    if (parent)
    {
        auto const wl_surface = WlSurface::from(*parent);
        wl_surface->add_subsurface(this);
    }

    auto const wl_surface = WlSurface::from(*surface);
    wl_surface->set_role(this);
    wl_surface->pending_invalidate_surface_data();
}

mf::WlSubsurface::~WlSubsurface()
{
    if (auto const locked_parent = parent.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_parent);
        wl_surface->remove_subsurface(this);
    }

    if (auto const locked_surface = surface.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_surface);
        wl_surface->clear_role();
    }
    refresh_surface_data_now();
}

void mf::WlSubsurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                             std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                                             geometry::Displacement const& parent_offset) const
{
    if (auto const locked_surface = surface.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_surface);
        if (wl_surface->buffer_size())
        {
            // surface is mapped
            wl_surface->populate_surface_data(buffer_streams, input_shape_accumulator, parent_offset);
        }
    }
}

auto mf::WlSubsurface::total_offset() const -> geom::Displacement
{
    if (auto const locked_parent = parent.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_parent);
        return wl_surface->offset();
    }

    return geom::Displacement{};
}

auto mf::WlSubsurface::synchronized() const -> bool
{
    bool parent_synchronized{false};
    if (auto const locked_parent = parent.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_parent);
        parent_synchronized = wl_surface->synchronized();
    }
    return synchronized_ || parent_synchronized;
}

auto mf::WlSubsurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    if (auto const locked_parent = parent.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_parent);
        return wl_surface->scene_surface();
    }

    return std::nullopt;
}

void mf::WlSubsurface::parent_has_committed()
{
    if (cached_state && synchronized())
    {
        if (auto const locked_surface = surface.lock())
        {
            auto const wl_surface = WlSurface::from(*locked_surface);
            wl_surface->commit(cached_state.value());
        }
        cached_state = std::nullopt;
    }
}

auto mf::WlSubsurface::subsurface_at(geom::Point point) -> std::optional<WlSurface*>
{
    if (auto const locked_surface = surface.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_surface);
        return wl_surface->subsurface_at(point);
    }
    return std::nullopt;
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    if (auto const locked_surface = surface.lock())
    {
        auto const wl_surface = WlSurface::from(*locked_surface);
        wl_surface->set_pending_offset(geom::Displacement(x, y));
    }
}

void mf::WlSubsurface::place_above(std::shared_ptr<wayland_rs::WlSurfaceImpl> const& sibling)
{
    auto const locked_parent = parent.lock();
    if (!locked_parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    if (sibling == surface.lock())
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_above: sibling cannot be the subsurface itself"));
    }

    auto parent_surface = WlSurface::from(*locked_parent);
    auto sibling_surface = WlSurface::from(*sibling);
    if (sibling != locked_parent && !parent_surface->has_subsurface_with_surface(sibling_surface))
    {
        BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_above: sibling must be the parent or a sibling subsurface"));
    }

    parent_surface->reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::above);
}

void mf::WlSubsurface::place_below(std::shared_ptr<wayland_rs::WlSurfaceImpl> const& sibling)
{
    auto const locked_parent = parent.lock();
    if (!locked_parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    if (sibling == surface.lock())
    {
        BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_below: sibling cannot be the subsurface itself"));
    }

    auto parent_surface = WlSurface::from(*locked_parent);
    auto sibling_surface = WlSurface::from(*sibling);
    if (sibling != locked_parent && !parent_surface->has_subsurface_with_surface(sibling_surface))
    {
        BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
            object_id(),
            mw::WlSubsurfaceImpl::Error::bad_surface,
            "wl_subsurface.place_below: sibling must be the parent or a sibling subsurface"));
    }

    parent_surface->reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::below);
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
    if (auto const locked_parent = parent.lock())
    {
        auto wl_surface = WlSurface::from(*locked_parent);
        wl_surface->refresh_surface_data_now();
    }
}

void mf::WlSubsurface::commit(WlSurfaceState const& state)
{
    if (!cached_state)
        cached_state = WlSurfaceState();

    cached_state.value().update_from(state);

    if (cached_state.value().buffer)
    {
        bool currently_mapped{false};
        if (auto const locked_surface = surface.lock())
        {
            auto const wl_surface = WlSurface::from(*locked_surface);
            currently_mapped = static_cast<bool>(wl_surface->buffer_size());
        }
        bool const pending_mapped = cached_state.value().buffer.value();
        if (currently_mapped != pending_mapped)
        {
            cached_state.value().invalidate_surface_data();
        }
    }

    if (synchronized())
    {
        if (cached_state.value().surface_data_needs_refresh())
        {
            if (auto const locked_parent = parent.lock())
            {
                auto wl_surface = WlSurface::from(*locked_parent);
                wl_surface->pending_invalidate_surface_data();
            }
        }
    }
    else
    {
        if (auto const locked_surface = surface.lock())
        {
            auto const wl_surface = WlSurface::from(*locked_surface);
            wl_surface->commit(cached_state.value());
        }

        if (cached_state.value().surface_data_needs_refresh())
            refresh_surface_data_now();
        cached_state = std::nullopt;
    }
}

void mf::WlSubsurface::surface_destroyed()
{
    if (surface.expired())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{
            "wl_surface destroyed before it's associated wl_subsurface@" + std::to_string(object_id())});
    }
}
