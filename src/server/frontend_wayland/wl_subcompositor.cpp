/*
 * Copyright © 2015-2018 Canonical Ltd.
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

#include "mir/log.h"

#include "wl_subcompositor.h"
#include "wl_surface.h"

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
    void destroy() override;
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

void mf::WlSubcompositorInstance::destroy()
{
    destroy_wayland_object();
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
      parent_destroyed{parent_surface->destroyed_flag()},
      synchronized_{true}
{
    parent->add_subsurface(this);
    surface->set_role(this);
    surface->pending_invalidate_surface_data();
}

mf::WlSubsurface::~WlSubsurface()
{
    if (!*parent_destroyed)
    {
        parent->remove_subsurface(this);
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

bool mf::WlSubsurface::synchronized() const
{
    return synchronized_ || parent->synchronized();
}

auto mf::WlSubsurface::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    return parent->scene_surface();
}

void mf::WlSubsurface::parent_has_committed()
{
    if (cached_state && synchronized())
    {
        surface->commit(cached_state.value());
        cached_state = std::experimental::nullopt;
    }
}

auto mf::WlSubsurface::subsurface_at(geom::Point point) -> std::experimental::optional<WlSurface*>
{
    return surface->subsurface_at(point);
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    surface->set_pending_offset(geom::Displacement{x, y});
}

void mf::WlSubsurface::place_above(struct wl_resource* sibling)
{
    (void)sibling;
    log_warning("TODO: wl_subsurface.place_above not implemented");
}

void mf::WlSubsurface::place_below(struct wl_resource* sibling)
{
    (void)sibling;
    log_warning("TODO: wl_subsurface.place_below not implemented");
}

void mf::WlSubsurface::set_sync()
{
    synchronized_ = true;
}

void mf::WlSubsurface::set_desync()
{
    synchronized_ = false;
}

void mf::WlSubsurface::destroy()
{
    destroy_wayland_object();
}

void mf::WlSubsurface::refresh_surface_data_now()
{
    if (!*parent_destroyed)
        parent->refresh_surface_data_now();
}

void mf::WlSubsurface::commit(WlSurfaceState const& state)
{
    if (!cached_state)
        cached_state = WlSurfaceState();

    cached_state.value().update_from(state);

    if (cached_state.value().buffer)
    {
        auto const currently_mapped = static_cast<bool>(surface->buffer_size());
        auto const pending_mapped = cached_state.value().buffer.value() != nullptr;
        if (currently_mapped != pending_mapped)
        {
            cached_state.value().invalidate_surface_data();
        }
    }

    if (synchronized())
    {
        if (cached_state.value().surface_data_needs_refresh() && !*parent_destroyed)
            parent->pending_invalidate_surface_data();
    }
    else
    {
        surface->commit(cached_state.value());
        if (cached_state.value().surface_data_needs_refresh())
            refresh_surface_data_now();
        cached_state = std::experimental::nullopt;
    }
}
