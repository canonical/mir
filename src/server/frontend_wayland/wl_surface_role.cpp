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

#include "wl_surface_role.h"

#include "wayland_utils.h"
#include "wl_surface.h"
#include "basic_surface_event_sink.h"

#include "mir/shell/surface_specification.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/event_sink.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

void mir::frontend::WlAbstractMirWindow::set_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_maximized);
}

void mir::frontend::WlAbstractMirWindow::unset_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_restored);
}

void mir::frontend::WlAbstractMirWindow::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    (void)output; // TODO specify the output when setting fullscreen
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_fullscreen);
}

void mir::frontend::WlAbstractMirWindow::unset_fullscreen()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_restored);
}

void mir::frontend::WlAbstractMirWindow::set_minimized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_minimized);
}

void mir::frontend::WlAbstractMirWindow::set_state_now(MirWindowState state)
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

namespace mir
{
namespace frontend
{

namespace
{
std::shared_ptr<scene::Surface> get_surface_for_id(std::shared_ptr<Session> const& session, SurfaceId surface_id)
{
    return std::dynamic_pointer_cast<scene::Surface>(session->get_surface(surface_id));
}
}

WlAbstractMirWindow::WlAbstractMirWindow(WlSeat* seat, wl_client* client, WlSurface* surface,
                                         std::shared_ptr<Shell> const& shell)
        : destroyed{std::make_shared<bool>(false)},
          client{client},
          surface{surface},
          shell{shell},
          sink{std::make_shared<BasicSurfaceEventSink>(seat, client, surface, this)},
          params{std::make_unique<scene::SurfaceCreationParameters>(
                 scene::SurfaceCreationParameters().of_type(mir_window_type_freestyle))}
{
}

WlAbstractMirWindow::~WlAbstractMirWindow()
{
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

void WlAbstractMirWindow::refresh_surface_data_now()
{
    shell::SurfaceSpecification buffer_list_spec;
    buffer_list_spec.streams = std::vector<shell::StreamSpecification>();
    surface->populate_surface_data(buffer_list_spec.streams.value(), {});
    shell->modify_surface(get_session(client), surface_id_, buffer_list_spec);
}

shell::SurfaceSpecification& WlAbstractMirWindow::spec()
{
    if (!pending_changes)
        pending_changes = std::make_unique<shell::SurfaceSpecification>();

    return *pending_changes;
}

void WlAbstractMirWindow::commit(WlSurfaceState const& state)
{
    surface->commit(state);

    auto const session = get_session(client);

    if (surface_id_.as_value())
    {
        auto const scene_surface = get_surface_for_id(session, surface_id_);

        sink->latest_client_size(window_size());

        if (!window_size_.is_set())
        {
            auto& new_size_spec = spec();
            new_size_spec.width = window_size().width;
            new_size_spec.height = window_size().height;
        }

        if (state.surface_data_needs_refresh())
        {
            auto& buffer_list_spec = spec();
            buffer_list_spec.streams = std::vector<shell::StreamSpecification>();
            surface->populate_surface_data(buffer_list_spec.streams.value(), {});
        }

        if (pending_changes)
            shell->modify_surface(session, surface_id_, *pending_changes);

        pending_changes.reset();
        return;
    }

    create_mir_window();
}

void WlAbstractMirWindow::create_mir_window()
{
    auto const session = get_session(client);

    if (params->size == geometry::Size{})
        params->size = window_size();
    if (params->size == geometry::Size{})
        params->size = geometry::Size{640, 480};

    params->streams = std::vector<shell::StreamSpecification>{};
    surface->populate_surface_data(params->streams.value(), {});

    surface_id_ = shell->create_surface(session, *params, sink);

    // The shell isn't guaranteed to respect the requested size
    auto const window = session->get_surface(surface_id_);
    auto const client_size = window->client_size();

    if (client_size != params->size)
        handle_resize(client_size);
}

geometry::Size WlAbstractMirWindow::window_size()
{
     return window_size_.is_set()? window_size_.value() : surface->buffer_size();
}

void WlAbstractMirWindow::visiblity(bool visible)
{
    if (!surface_id_.as_value())
        return;

    auto const session = get_session(client);

    auto mir_surface = get_surface_for_id(session, surface_id_);

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

}
}
