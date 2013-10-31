/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"

#include "mir/shell/broadcasting_session_event_sink.h"
#include "mir/shell/organising_surface_factory.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/surface_source.h"

namespace msh = mir::shell;
namespace mf = mir::frontend;

std::shared_ptr<msh::SessionManager>
mir::DefaultServerConfiguration::the_session_manager()
{
    return session_manager(
        [this]() -> std::shared_ptr<msh::SessionManager>
        {
            return std::make_shared<msh::SessionManager>(
                the_shell_surface_factory(),
                the_shell_session_container(),
                the_shell_focus_sequence(),
                the_shell_focus_setter(),
                the_shell_snapshot_strategy(),
                the_shell_session_event_sink(),
                the_shell_session_listener());
        });
}

std::shared_ptr<mf::Shell>
mir::DefaultServerConfiguration::the_frontend_shell()
{
    return the_session_manager();
}

std::shared_ptr<msh::FocusController>
mir::DefaultServerConfiguration::the_focus_controller()
{
    return the_session_manager();
}

std::shared_ptr<msh::SurfaceFactory>
mir::DefaultServerConfiguration::the_shell_surface_factory()
{
    return shell_surface_factory(
        [this]()
        {
            auto surface_source = std::make_shared<msh::SurfaceSource>(
                the_surface_builder(), the_shell_surface_configurator());

            return std::make_shared<msh::OrganisingSurfaceFactory>(
                surface_source,
                the_shell_placement_strategy());
        });
}

std::shared_ptr<msh::BroadcastingSessionEventSink>
mir::DefaultServerConfiguration::the_broadcasting_session_event_sink()
{
    return broadcasting_session_event_sink(
        []
        {
            return std::make_shared<msh::BroadcastingSessionEventSink>();
        });
}

std::shared_ptr<msh::SessionEventSink>
mir::DefaultServerConfiguration::the_shell_session_event_sink()
{
    return the_broadcasting_session_event_sink();
}

std::shared_ptr<msh::SessionEventHandlerRegister>
mir::DefaultServerConfiguration::the_shell_session_event_handler_register()
{
    return the_broadcasting_session_event_sink();
}
