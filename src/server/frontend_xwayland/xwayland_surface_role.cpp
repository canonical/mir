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

#include "xwayland_surface_role.h"
#include "xwayland_surface_role_surface.h"
#include "scaled_buffer_stream.h"
#include "xwayland_log.h"

#include "wl_surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
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

void mf::XWaylandSurfaceRole::populate_surface_data_scaled(
    WlSurface* wl_surface,
    float scale,
    shell::SurfaceSpecification& spec,
    std::vector<std::shared_ptr<void>>& keep_alive_until_spec_is_used)
{
    spec.streams = std::vector<shell::StreamSpecification>();
    spec.input_shape = std::vector<geom::Rectangle>();
    wl_surface->populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});

    if (scale != 1.0f)
    {
        auto const inv_scale = 1.0f / scale;

        for (auto& stream : spec.streams.value())
        {
            if (auto inner = std::dynamic_pointer_cast<mc::BufferStream>(stream.stream.lock()))
            {
                // We could set the scale of the original stream, but then the WlSurface would see the scaled size.
                // Instead, we wrap the surface's stream in our own that scales without the surface knowing.
                auto scaled = std::make_shared<ScaledBufferStream>(std::move(inner), scale);
                stream.stream = scaled;
                keep_alive_until_spec_is_used.push_back(std::move(scaled));
            }
            stream.displacement = stream.displacement * inv_scale;
        }

        for (auto& rect : spec.input_shape.value())
        {
            rect.top_left = as_point(as_displacement(rect.top_left) * inv_scale);
            rect.size = rect.size;
        }
    }
}

auto mf::XWaylandSurfaceRole::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    if (auto const wm_surface = weak_wm_surface.lock())
        return wm_surface->scene_surface();
    else
        return std::nullopt;
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
    std::vector<std::shared_ptr<void>> keep_alive_until_spec_is_used;
    populate_surface_data_scaled(wl_surface, scale, spec, keep_alive_until_spec_is_used);
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

        std::vector<std::shared_ptr<void>> keep_alive_until_spec_is_used;
        if (state.surface_data_needs_refresh())
        {
            populate_surface_data_scaled(wl_surface, scale, spec, keep_alive_until_spec_is_used);
        }

        if (!spec.is_empty())
        {
            shell->modify_surface(session, surface.value(), spec);
        }

        if (should_be_mapped && !surface_marked_ready)
        {
            shell->surface_ready(surface.value());
            surface_marked_ready = true;
        }
    }
}

void mf::XWaylandSurfaceRole::surface_destroyed()
{
    if (auto const wm_surface = weak_wm_surface.lock())
    {
        wm_surface->wl_surface_destroyed();
    }

    delete this;
}
