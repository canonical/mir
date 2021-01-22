/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "xwayland_surface_role.h"
#include "xwayland_surface_role_surface.h"
#include "xwayland_log.h"

#include "wl_surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::XWaylandSurfaceRole::XWaylandSurfaceRole(
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<XWaylandSurfaceRoleSurface> const& wm_surface,
    WlSurface* wl_surface,
    float scale)
    : shell{shell},
      weak_wm_surface{wm_surface},
      wl_surface{wl_surface},
      scale{scale}
{
    wl_surface->set_role(this);
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("Created XWaylandSurfaceRole for wl_surface@%d", wl_resource_get_id(wl_surface->resource));
    }
}

mf::XWaylandSurfaceRole::~XWaylandSurfaceRole()
{
    wl_surface->clear_role();
    if (verbose_xwayland_logging_enabled())
    {
        log_debug("Destroyed XWaylandSurfaceRole for wl_surface@%d", wl_resource_get_id(wl_surface->resource));
    }
}

void mf::XWaylandSurfaceRole::populate_surface_data(shell::SurfaceSpecification& spec)
{
    spec.streams = std::vector<shell::StreamSpecification>();
    spec.input_shape = std::vector<geom::Rectangle>();
    wl_surface->populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});

    if (scale == 1.0f)
    {
        return;
    }

    auto const inv_scale = 1.0f / scale;

    for (auto& stream : spec.streams.value())
    {
        if (auto const bs = stream.stream.lock())
        {
            bs->set_scale(scale);
        }
        if (stream.size)
        {
            stream.size = stream.size.value() * inv_scale;
        }
        stream.displacement = stream.displacement * inv_scale;
    }

    // we *should* be scaling the input shape, however when it's not set explicitly (which it doesn't seem to be for
    // XWayland apps) the surface is setting it based on the buffer size. Since we're scaling the buffer stream, it's
    // seeing it's buffer size in Mir coordinates rather than XWayland ones (it should be XWayland). This might cause
    // other problems (especially around selecting the right subsurface to dispatch input to), but that doesn't seem to
    // be an issue at this time (probably because XWayland doesn't make subsurfaces either). Anyway, we need to sort
    // this all out but it makes sense to wait until we've made subsurfaces real surfaces and simplified all that logic.
    /*
    for (auto& rect : spec.input_shape.value())
    {
        rect.top_left = as_point(as_displacement(rect.top_left) * inv_scale);
        rect.size = rect.size * inv_scale;
    }
    */
}

auto mf::XWaylandSurfaceRole::scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>>
{
    if (auto const wm_surface = weak_wm_surface.lock())
        return wm_surface->scene_surface();
    else
        return std::experimental::nullopt;
}

void mf::XWaylandSurfaceRole::refresh_surface_data_now()
{
    if (!wl_surface)
        return;

    auto const surface = scene_surface();
    if (!surface)
        return;

    auto const session = surface.value()->session().lock();
    if (!session)
        return;

    shell::SurfaceSpecification spec;
    populate_surface_data(spec);
    shell->modify_surface(session, surface.value(), spec);
}

void mf::XWaylandSurfaceRole::commit(WlSurfaceState const& state)
{
    if (!wl_surface)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Got XWaylandSurfaceRole::commit() when the role had no surface"));
    }

    wl_surface->commit(state);

    auto const surface = this->scene_surface();
    auto const session = surface ? surface.value()->session().lock() : nullptr;
    if (surface && session)
    {
        shell::SurfaceSpecification spec;

        bool const is_mapped = surface.value()->visible();
        bool const should_be_mapped = static_cast<bool>(wl_surface->buffer_size());
        if (!is_mapped && should_be_mapped && surface.value()->state() == mir_window_state_hidden)
        {
            spec.state = mir_window_state_restored;
        }
        else if (is_mapped && !should_be_mapped)
        {
            spec.state = mir_window_state_hidden;
        }

        if (state.surface_data_needs_refresh())
        {
            populate_surface_data(spec);
        }

        if (!spec.is_empty())
        {
            shell->modify_surface(session, surface.value(), spec);
        }
    }
}

void mf::XWaylandSurfaceRole::destroy()
{
    if (auto const wm_surface = weak_wm_surface.lock())
    {
        wm_surface->wl_surface_destroyed();
    }

    delete this;
}
