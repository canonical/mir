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

#include "default_shell.h"
#include "default_window_manager.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

msh::DefaultShell::DefaultShell(
    std::shared_ptr<InputTargeter> const& input_targeter,
    std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
    std::shared_ptr<ms::PlacementStrategy> const& placement_strategy,
    std::shared_ptr<ms::SurfaceConfigurator> const& surface_configurator) :
    AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager,
        [&](FocusController* focus_controller) { return std::make_shared<DefaultWindowManager>(focus_controller, placement_strategy, session_coordinator, surface_configurator); })
{
}
