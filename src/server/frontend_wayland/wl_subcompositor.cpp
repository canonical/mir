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

#include <mir/wayland/client.h>
#include <mir/geometry/rectangle.h>
#include <boost/throw_exception.hpp>

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_rs/wayland_rs_cpp/include/protocol_error.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

mf::WlSubcompositor::WlSubcompositor(rust::Box<wayland_rs::WaylandClient> client)
    : WlSubcompositorImpl(std::move(client))
{
}

auto mf::WlSubcompositor::get_subsurface(
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent)
    -> std::shared_ptr<wayland_rs::WlSubsurfaceImpl>
{
    return std::make_shared<WlSubsurface>(client->clone_box(), surface, parent);
}

mf::WlSubsurface::WlSubsurface(
    rust::Box<wayland_rs::WaylandClient> client,
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent)
    : wayland_rs::WlSubsurfaceImpl(std::move(client)),
      surface{surface},
      parent{parent},
      synchronized_{true}
{
    if (parent)
    {
        parent.with_value([this](auto& parent)
        {
            auto const wl_surface = WlSurface::from(parent);
            wl_surface->add_subsurface(this);
        });
    }

    surface.with_value([this](auto& surface)
    {
        auto const wl_surface = WlSurface::from(surface);
        wl_surface->set_role(this);
        wl_surface->pending_invalidate_surface_data();
    });
}

mf::WlSubsurface::~WlSubsurface()
{
    if (parent)
    {
        parent.with_value([this](auto& captured_parent)
        {
            auto const wl_surface = WlSurface::from(captured_parent);
            wl_surface->remove_subsurface(this);
        });
    }

    surface.with_value([](auto& captured_surface)
    {
        auto const wl_surface = WlSurface::from(captured_surface);
        wl_surface->clear_role();
    });
    refresh_surface_data_now();
}

void mf::WlSubsurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                             std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                                             geometry::Displacement const& parent_offset) const
{
    surface.with_value([&](auto& captured_surface)
    {
        auto const wl_surface = WlSurface::from(captured_surface);
        if (wl_surface->buffer_size())
        {
            // surface is mapped
            wl_surface->populate_surface_data(buffer_streams, input_shape_accumulator, parent_offset);
        }
    });
}

auto mf::WlSubsurface::total_offset() const -> geom::Displacement
{
    if (parent)
    {
        return parent.with_value([](auto& captured_parent)
        {
            auto const wl_surface = WlSurface::from(captured_parent);
            return wl_surface->offset();
        });
    }

    return geom::Displacement{};
}

auto mf::WlSubsurface::synchronized() const -> bool
{
    bool const parent_synchronized = parent && parent.with_value([](auto& captured_parent)
    {
        auto const wl_surface = WlSurface::from(captured_parent);
        return wl_surface->synchronized();
    });
    return synchronized_ || parent_synchronized;
}

auto mf::WlSubsurface::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    if (parent)
    {
        return parent.with_value([](auto& captured_parent)
        {
            auto const wl_surface = WlSurface::from(captured_parent);
            return wl_surface->scene_surface();
        });
    }

    return std::nullopt;
}

void mf::WlSubsurface::parent_has_committed()
{
    if (cached_state && synchronized())
    {
        surface.with_value([&](auto& captured_surface)
        {
            auto const wl_surface = WlSurface::from(captured_surface);
            wl_surface->commit(cached_state.value());
        });
        cached_state = std::nullopt;
    }
}

auto mf::WlSubsurface::subsurface_at(geom::Point point) -> std::optional<WlSurface*>
{
    return surface.with_value([&point](auto& captured)
    {
        auto const wl_surface = WlSurface::from(captured);
        return wl_surface->subsurface_at(point);
    });
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    surface.with_value([x, y](auto& captured)
    {
        auto const wl_surface = WlSurface::from(captured);
        wl_surface->set_pending_offset(geom::Displacement(x, y));
    });
}

void mf::WlSubsurface::place_above(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling)
{
    if (!parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    if (sibling == surface)
    {
        BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
            object_id(),
            mw::Subsurface::Error::bad_surface,
            "wl_subsurface.place_above: sibling cannot be the subsurface itself"));
    }

    parent.with_value([&](auto& captured_parent)
    {
        auto parent_surface = WlSurface::from(captured_parent);
        sibling.with_value([&](auto& captured_sibling)
        {
            auto sibling_surface = WlSurface::from(captured_sibling);
            if (sibling != parent && !parent_surface->has_subsurface_with_surface(sibling_surface))
            {
                BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
                    object_id(),
                    mw::Subsurface::Error::bad_surface,
                    "wl_subsurface.place_above: sibling must be the parent or a sibling subsurface"));
            }

            parent_surface->reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::above);
        });
    });
}

void mf::WlSubsurface::place_below(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling)
{
    if (!parent)
    {
        // Parent has been destroyed, ignore
        return;
    }

    if (sibling == surface)
    {
        BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
            object_id(),
            mw::Subsurface::Error::bad_surface,
            "wl_subsurface.place_below: sibling cannot be the subsurface itself"));
    }

    parent.with_value([&](auto& captured_parent)
    {
        auto parent_surface = WlSurface::from(captured_parent);
        sibling.with_value([&](auto& captured_sibling)
        {
            auto sibling_surface = WlSurface::from(captured_sibling);
            if (sibling != parent && !parent_surface->has_subsurface_with_surface(sibling_surface))
            {
                BOOST_THROW_EXCEPTION(wayland_rs::ProtocolError(
                    object_id(),
                    mw::Subsurface::Error::bad_surface,
                    "wl_subsurface.place_above: sibling must be the parent or a sibling subsurface"));
            }

            parent_surface->reorder_subsurface(this, sibling_surface, WlSurface::SubsurfacePlacement::below);
        });
    });
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
        parent.with_value([](auto& captured)
        {
            auto wl_surface = WlSurface::from(captured);
            wl_surface->refresh_surface_data_now();
        });
    }
}

void mf::WlSubsurface::commit(WlSurfaceState const& state)
{
    if (!cached_state)
        cached_state = WlSurfaceState();

    cached_state.value().update_from(state);

    if (cached_state.value().buffer)
    {
        bool const currently_mapped =  surface.with_value([&](auto& captured_surface)
        {
            auto const wl_surface = WlSurface::from(captured_surface);
            return static_cast<bool>(wl_surface->buffer_size());
        });
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
            parent.with_value([](auto& captured)
            {
                auto wl_surface = WlSurface::from(captured);
                wl_surface->pending_invalidate_surface_data();
            });
        }
    }
    else
    {
        surface.with_value([&](auto& captured_surface)
        {
            auto const wl_surface = WlSurface::from(captured_surface);
            wl_surface->commit(cached_state.value());
        });

        if (cached_state.value().surface_data_needs_refresh())
            refresh_surface_data_now();
        cached_state = std::nullopt;
    }
}

void mf::WlSubsurface::surface_destroyed()
{
    if (!surface)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{
            "wl_surface destroyed before it's associated wl_subsurface@" + std::to_string(object_id())});
    }
}
