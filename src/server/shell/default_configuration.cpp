/*
 * Copyright © 2013-2015 Canonical Ltd.
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
#include "null_host_lifecycle_event_listener.h"


#include "default_placement_strategy.h"
#include "default_shell.h"
#include "graphics_display_layout.h"

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;

auto mir::DefaultServerConfiguration::the_shell() -> std::shared_ptr<msh::DefaultShell>
{
    return default_shell([this]
        {
            return std::make_shared<msh::DefaultShell>(
                the_input_targeter(),
                the_surface_coordinator(),
                the_session_coordinator());
        });
}

std::shared_ptr<mf::Shell>
mir::DefaultServerConfiguration::the_frontend_shell()
{
    return the_shell();
}


std::shared_ptr<msh::FocusController>
mir::DefaultServerConfiguration::the_focus_controller()
{
    return the_shell();
}

std::shared_ptr<ms::PlacementStrategy>
mir::DefaultServerConfiguration::the_placement_strategy()
{
    return shell_placement_strategy(
        [this]
        {
            return std::make_shared<msh::DefaultPlacementStrategy>(
                the_shell_display_layout());
        });
}

std::shared_ptr<msh::DisplayLayout>
mir::DefaultServerConfiguration::the_shell_display_layout()
{
    return shell_display_layout(
        [this]()
        {
            return std::make_shared<msh::GraphicsDisplayLayout>(the_display());
        });
}

std::shared_ptr<msh::HostLifecycleEventListener>
mir::DefaultServerConfiguration::the_host_lifecycle_event_listener()
{
    return host_lifecycle_event_listener(
        []()
        {
            return std::make_shared<msh::NullHostLifecycleEventListener>();
        });
}
