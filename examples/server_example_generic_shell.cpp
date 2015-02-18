/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_generic_shell.h"

#include "mir/scene/session.h"
#include "mir/scene/surface_creation_parameters.h"

namespace me = mir::examples;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

///\example server_example_generic_shell.cpp
/// A shell that accepts a WindowManager

me::GenericShell::GenericShell(
    std::shared_ptr<msh::InputTargeter> const& input_targeter,
    std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<ms::PromptSessionManager> const& prompt_session_manager,
    std::function<std::shared_ptr<WindowManager>(shell::FocusController* focus_controller)> const& wm_builder) :
    AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager),
    window_manager(wm_builder(this))
{
}

std::shared_ptr<ms::Session> me::GenericShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    auto const result = msh::AbstractShell::open_session(client_pid, name, sink);
    window_manager->add_session(result);
    return result;
}

void me::GenericShell::close_session(std::shared_ptr<ms::Session> const& session)
{
    window_manager->remove_session(session);
    msh::AbstractShell::close_session(session);
}

auto me::GenericShell::create_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)
-> mf::SurfaceId
{
    auto const build = [this](std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& placed_params)
        {
            return msh::AbstractShell::create_surface(session, placed_params);
        };

    return window_manager->add_surface(session, params, build);
}

void me::GenericShell::destroy_surface(std::shared_ptr<ms::Session> const& session, mf::SurfaceId surface)
{
    window_manager->remove_surface(session->surface(surface), session);
    msh::AbstractShell::destroy_surface(session, surface);
}

bool me::GenericShell::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    auto const input_event = mir_event_get_input_event(&event);

    switch (mir_input_event_get_type(input_event))
    {
    case mir_input_event_type_key:
        return window_manager->handle_key_event(mir_input_event_get_key_input_event(input_event));

    case mir_input_event_type_touch:
        return window_manager->handle_touch_event(mir_input_event_get_touch_input_event(input_event));

    case mir_input_event_type_pointer:
        return window_manager->handle_pointer_event(mir_input_event_get_pointer_input_event(input_event));
    }

    return false;
}

int me::GenericShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    switch (attrib)
    {
    case mir_surface_attrib_state:
    {
        auto const state = window_manager->handle_set_state(surface, MirSurfaceState(value));
        return msh::AbstractShell::set_surface_attribute(session, surface, attrib, state);
    }
    default:
        return msh::AbstractShell::set_surface_attribute(session, surface, attrib, value);
    }
}

void me::GenericShell::add_display(geometry::Rectangle const& area)
{
    window_manager->add_display(area);
}

void me::GenericShell::remove_display(geometry::Rectangle const& area)
{
    window_manager->remove_display(area);
}
