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
}

mf::WindowWlSurfaceRole::~WindowWlSurfaceRole()
{
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

void mf::WindowWlSurfaceRole::set_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_maximized);
}

void mf::WindowWlSurfaceRole::unset_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_restored);
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

void mf::WindowWlSurfaceRole::unset_fullscreen()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_restored);
}

void mf::WindowWlSurfaceRole::set_minimized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_minimized);
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

geom::Size mf::WindowWlSurfaceRole::window_size()
{
     return window_size_.is_set()? window_size_.value() : surface->buffer_size();
}

mir::shell::SurfaceSpecification& mf::WindowWlSurfaceRole::spec()
{
    if (!pending_changes)
        pending_changes = std::make_unique<mir::shell::SurfaceSpecification>();

    return *pending_changes;
}

void mf::WindowWlSurfaceRole::commit(WlSurfaceState const& state)
{
    surface->commit(state);

    auto const session = get_session(client);

    if (surface_id_.as_value())
    {
        auto const scene_surface = scene_surface_from(session, surface_id_);

        sink->latest_client_size(window_size());

        if (!window_size_.is_set())
        {
            auto& new_size_spec = spec();
            new_size_spec.width = window_size().width;
            new_size_spec.height = window_size().height;
        }

        if (state.surface_data_needs_refresh())
        {
            populate_spec_with_surface_data(spec());
        }

        if (pending_changes)
            shell->modify_surface(session, surface_id_, *pending_changes);

        pending_changes.reset();
        return;
    }

    create_mir_window();
}

void mf::WindowWlSurfaceRole::create_mir_window()
{
    auto const session = get_session(client);

    if (params->size == geometry::Size{})
        params->size = window_size();
    if (params->size == geometry::Size{})
        params->size = geometry::Size{640, 480};

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

