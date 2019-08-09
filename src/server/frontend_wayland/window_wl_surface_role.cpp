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
#include "mir/frontend/wayland.h"
#include "mir/frontend/mir_client_session.h"
#include "mir/frontend/event_sink.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
std::shared_ptr<ms::Surface> scene_surface_from(std::shared_ptr<mf::MirClientSession> const& session, mf::SurfaceId surface_id)
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
        if (auto session = get_mir_client_session(client))
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
    shell->modify_surface(get_mir_client_session(client), surface_id_, surface_data_spec);
}

void mf::WindowWlSurfaceRole::apply_spec(mir::shell::SurfaceSpecification const& new_spec)
{
    if (new_spec.width.is_set())
        pending_explicit_width = new_spec.width.value();
    if (new_spec.height.is_set())
        pending_explicit_height = new_spec.height.value();

    if (surface_id().as_value())
    {
        spec().update_from(new_spec);
    }
    else
    {
        params->update_from(new_spec);
    }
}

void mf::WindowWlSurfaceRole::set_pending_offset(std::experimental::optional<geom::Displacement> const& offset)
{
    surface->set_pending_offset(offset);
}

void mf::WindowWlSurfaceRole::set_pending_width(std::experimental::optional<geometry::Width> const& width)
{
    pending_explicit_width = width;
}

void mf::WindowWlSurfaceRole::set_pending_height(std::experimental::optional<geometry::Height> const& height)
{
    pending_explicit_height = height;
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
        if (auto session = get_mir_client_session(client))
        {
            shell->request_operation(session, surface_id(), sink->latest_timestamp_ns(), Shell::UserRequest::move);
        }
    }
}

void mf::WindowWlSurfaceRole::initiate_interactive_resize(MirResizeEdge edge)
{
    if (surface_id().as_value())
    {
        if (auto session = get_mir_client_session(client))
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
        auto const session = get_mir_client_session(client);
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
        auto const session = get_mir_client_session(client);
        shell->modify_surface(session, surface_id_, mods);
    }
    else
    {
        params->state = state;
        create_mir_window();
    }
}

auto mf::WindowWlSurfaceRole::pending_size() const -> geom::Size
{
    auto size = current_size();
    if (pending_explicit_width)
        size.width = pending_explicit_width.value();
    if (pending_explicit_height)
        size.height = pending_explicit_height.value();
    return size;
}

auto mf::WindowWlSurfaceRole::current_size() const -> geom::Size
{
    auto size = committed_size.value_or(geom::Size{640, 480});
    if (surface->buffer_size())
    {
        if (!committed_width_set_explicitly)
            size.width = surface->buffer_size().value().width;
        if (!committed_height_set_explicitly)
            size.height = surface->buffer_size().value().height;
    }
    return size;
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

    handle_commit();

    auto const session = get_mir_client_session(client);
    auto size = pending_size();
    sink->latest_client_size(size);

    if (surface_id_.as_value())
    {
        auto const scene_surface = scene_surface_from(session, surface_id_);

        if (!committed_size || size != committed_size.value())
        {
            spec().width = size.width;
            spec().height = size.height;
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

    committed_size = size;
    if (pending_explicit_width)
        committed_width_set_explicitly = true;
    if (pending_explicit_height)
        committed_height_set_explicitly = true;
    pending_explicit_width = std::experimental::nullopt;
    pending_explicit_height = std::experimental::nullopt;
}

void mf::WindowWlSurfaceRole::visiblity(bool visible)
{
    if (!surface_id_.as_value())
        return;

    auto const session = get_mir_client_session(client);

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
    auto const session = get_mir_client_session(client);

    params->size = pending_size();
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

