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

#include "window_wl_surface_role.h"

#include "output_manager.h"
#include "wayland_utils.h"
#include "wl_surface.h"
#include "wl_surface_event_sink.h"
#include "wl_seat.h"

#include "mir/shell/surface_specification.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/event_sink.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
std::shared_ptr<ms::Surface> scene_surface_from(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface_id)
{
    return std::dynamic_pointer_cast<ms::Surface>(session->get_surface(surface_id));
}
}

mf::WindowWlSurfaceRole::WindowWlSurfaceRole(WlSeat* seat, wl_client* client, WlSurface* surface,
                                             std::shared_ptr<Shell> const& shell, OutputManager* output_manager)
        : destroyed{std::make_shared<bool>(false)},
          client{client},
          surface{surface},
          shell{shell},
          output_manager{output_manager},
          sink{std::make_shared<WlSurfaceEventSink>(seat, client, surface, this)},
          params{std::make_unique<scene::SurfaceCreationParameters>(
                 scene::SurfaceCreationParameters().of_type(mir_window_type_freestyle))}
{
    surface->set_role(this);
}

mf::WindowWlSurfaceRole::~WindowWlSurfaceRole()
{
    surface->clear_role();
    sink->disconnect();
    *destroyed = true;
    if (surface_id_.as_value())
    {
        if (auto session = get_session(client))
        {
            shell->destroy_surface(session, surface_id_);
        }

        surface_id_ = {};
    }
}

void mf::WindowWlSurfaceRole::populate_spec_with_surface_data(shell::SurfaceSpecification& spec)
{
    spec.streams = std::vector<shell::StreamSpecification>();
    spec.input_shape = std::vector<geom::Rectangle>();
    surface->populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});
}

void mf::WindowWlSurfaceRole::refresh_surface_data_now()
{
    shell::SurfaceSpecification surface_data_spec;
    populate_spec_with_surface_data(surface_data_spec);
    shell->modify_surface(get_session(client), surface_id_, surface_data_spec);
}

void mf::WindowWlSurfaceRole::apply_spec(mir::shell::SurfaceSpecification const& new_spec)
{
    if (surface_id().as_value())
    {
        spec().update_from(new_spec);
    }
    else
    {
        params->update_from(new_spec);
    }
}

void mf::WindowWlSurfaceRole::set_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    surface->set_pending_offset({-x, -y});
    pending_window_size = geom::Size{width, height};
}

void mf::WindowWlSurfaceRole::set_title(std::string const& title)
{
    if (surface_id().as_value())
    {
        spec().name = title;
    }
    else
    {
        params->name = title;
    }
}

void mf::WindowWlSurfaceRole::initiate_interactive_move()
{
    if (surface_id().as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(session, surface_id(), sink->latest_timestamp_ns(), Shell::UserRequest::move);
        }
    }
}

void mf::WindowWlSurfaceRole::initiate_interactive_resize(MirResizeEdge edge)
{
    if (surface_id().as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(
                session,
                surface_id(),
                sink->latest_timestamp_ns(),
                Shell::UserRequest::resize,
                edge);
        }
    }
}

void mf::WindowWlSurfaceRole::set_parent(optional_value<SurfaceId> parent_id)
{
    if (surface_id().as_value())
    {
        spec().parent_id = parent_id;
    }
    else
    {
        params->parent_id = parent_id;
    }
}

void mf::WindowWlSurfaceRole::set_max_size(int32_t width, int32_t height)
{
    if (surface_id().as_value())
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

void mf::WindowWlSurfaceRole::set_min_size(int32_t width, int32_t height)
{
    if (surface_id().as_value())
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

void mf::WindowWlSurfaceRole::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    // We must process this request immediately (i.e. don't defer until commit())
    if (surface_id_.as_value())
    {
        shell::SurfaceSpecification mods;
        mods.state = mir_window_state_fullscreen;
        mods.output_id = output_manager->output_id_for(client, output);
        auto const session = get_session(client);
        shell->modify_surface(session, surface_id_, mods);
    }
    else
    {
        params->state = mir_window_state_fullscreen;
        if (output)
            params->output_id = output_manager->output_id_for(client, output.value());
        create_mir_window();
    }
}

void mf::WindowWlSurfaceRole::set_state_now(MirWindowState state)
{
    if (surface_id_.as_value())
    {
        shell::SurfaceSpecification mods;
        mods.state = state;
        auto const session = get_session(client);
        shell->modify_surface(session, surface_id_, mods);
    }
    else
    {
        params->state = state;
        create_mir_window();
    }
}

std::experimental::optional<geom::Size> mf::WindowWlSurfaceRole::window_size() const
{
    if (pending_window_size)
        return pending_window_size;
    else if (committed_window_size)
        return committed_window_size;
    else
        return surface->buffer_size();
}

std::experimental::optional<geom::Size> mf::WindowWlSurfaceRole::requested_window_size()
{
    return sink->requested_window_size();
}

MirWindowState mf::WindowWlSurfaceRole::window_state()
{
    return sink->state();
}

bool mf::WindowWlSurfaceRole::is_active()
{
    return sink->is_active();
}

uint64_t mf::WindowWlSurfaceRole::latest_timestamp_ns()
{
    return sink->latest_timestamp_ns();
}

void mf::WindowWlSurfaceRole::commit(WlSurfaceState const& state)
{
    surface->commit(state);

    auto const session = get_session(client);

    if (surface_id_.as_value())
    {
        auto const scene_surface = scene_surface_from(session, surface_id_);

        if (window_size())
            sink->latest_client_size(window_size().value());

        auto size = window_size();
        if (size && size != committed_window_size)
        {
            spec().width = size.value().width;
            spec().height = size.value().height;
        }

        if (state.surface_data_needs_refresh())
        {
            populate_spec_with_surface_data(spec());
        }

        if (pending_changes)
            shell->modify_surface(session, surface_id_, *pending_changes);

        pending_changes.reset();
    }
    else
    {
        create_mir_window();
    }

    if (pending_window_size)
        committed_window_size = pending_window_size;

    pending_window_size = std::experimental::nullopt;
}

void mf::WindowWlSurfaceRole::visiblity(bool visible)
{
    if (!surface_id_.as_value())
        return;

    auto const session = get_session(client);

    auto mir_surface = scene_surface_from(session, surface_id_);

    if (mir_surface->visible() == visible)
        return;

    if (visible)
    {
        if (mir_surface->state() == mir_window_state_hidden)
            spec().state = mir_window_state_restored;
    }
    else
    {
        if (mir_surface->state() != mir_window_state_hidden)
            spec().state = mir_window_state_hidden;
    }
}

mir::shell::SurfaceSpecification& mf::WindowWlSurfaceRole::spec()
{
    if (!pending_changes)
        pending_changes = std::make_unique<mir::shell::SurfaceSpecification>();

    return *pending_changes;
}

void mf::WindowWlSurfaceRole::create_mir_window()
{
    auto const session = get_session(client);

    if (params->size == geometry::Size{})
        params->size = window_size().value_or(geometry::Size{640, 480});

    params->streams = std::vector<shell::StreamSpecification>{};
    params->input_shape = std::vector<geom::Rectangle>{};
    surface->populate_surface_data(params->streams.value(), params->input_shape.value(), {});

    surface_id_ = shell->create_surface(session, *params, sink);

    // The shell isn't guaranteed to respect the requested size
    auto const window = session->get_surface(surface_id_);
    auto const client_size = window->client_size();

    if (client_size != params->size)
        sink->handle_resize(client_size);
}

