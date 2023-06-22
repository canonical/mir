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

#include "mir/log.h"

#include "wl_subcompositor.h"
#include "wl_surface.h"

#include "mir/wayland/client.h"
#include "mir/geometry/rectangle.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
class WlSubcompositorInstance: wayland::Subcompositor
{
public:
    WlSubcompositorInstance(wl_resource* new_resource);

private:
    void get_subsurface(wl_resource* new_subsurface, wl_resource* surface, wl_resource* parent) override;
};
}
}

mf::WlSubcompositor::WlSubcompositor(wl_display* display)
    : Global{display, Version<1>()}
{
}

void mf::WlSubcompositor::bind(wl_resource* new_wl_subcompositor)
{
    new WlSubcompositorInstance(new_wl_subcompositor);
}

mf::WlSubcompositorInstance::WlSubcompositorInstance(wl_resource* new_resource)
    : wayland::Subcompositor(new_resource, Version<1>())
{
}

void mf::WlSubcompositorInstance::get_subsurface(
    wl_resource* new_subsurface,
    wl_resource* surface,
    wl_resource* parent)
{
    new WlSubsurface(new_subsurface, WlSurface::from(surface), WlSurface::from(parent));
}

mf::WlSubsurface::WlSubsurface(wl_resource* new_subsurface, WlSurface* surface, WlSurface* parent_surface)
    : wayland::Subsurface(new_subsurface, Version<1>()),
      surface{surface},
      parent{parent_surface},
      synchronized_{true}
{
    parent_surface->add_subsurface(this);
    surface->set_role(this);
    surface->pending_invalidate_surface_data();
}

mf::WlSubsurface::~WlSubsurface()
{
    if (parent)
    {
        parent.value().remove_subsurface(this);
    }
    surface->clear_role();
    refresh_surface_data_now();
}

void mf::WlSubsurface::populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                                             std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                                             geometry::Displacement const& parent_offset) const
{
    if (surface->buffer_size())
    {
        // surface is mapped
        surface->populate_surface_data(buffer_streams, input_shape_accumulator, parent_offset);
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
        surface->commit(cached_state.value());
        cached_state = std::nullopt;
    }
}

auto mf::WlSubsurface::subsurface_at(geom::Point point) -> std::optional<WlSurface*>
{
    return surface->subsurface_at(point);
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    surface->set_pending_offset(geom::Displacement{x, y});
}

void mf::WlSubsurface::place_above(struct wl_resource* sibling)
{
    if (parent)
    {
        auto const sibling_surface = WlSurface::from(sibling);
        parent.value().reorder_subsurface(this, sibling_surface, true);
    }
    surface->pending_invalidate_surface_data();
}

void mf::WlSubsurface::place_below(struct wl_resource* sibling)
{
    if (parent)
    {
        auto const sibling_surface = WlSurface::from(sibling);
        parent.value().reorder_subsurface(this, sibling_surface, false);
    }
    surface->pending_invalidate_surface_data();
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
        bool const currently_mapped = static_cast<bool>(surface->buffer_size());
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
        surface->commit(cached_state.value());
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
            "wl_surface@" + std::to_string(wl_resource_get_id(surface->resource)) +
            " destroyed before it's associated wl_subsurface@" + std::to_string(wl_resource_get_id(resource))});
    }
    else
    {
        // If the client has been destroyed, everything is getting cleaned up in an arbitrary order. Delete this so our
        // derived class doesn't end up using the now-defunct surface.
        delete this;
    }
}
