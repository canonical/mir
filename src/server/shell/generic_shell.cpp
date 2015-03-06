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

#include "mir/shell/generic_shell.h"
#include "mir/shell/window_manager.h"

#include "mir/geometry/point.h"
#include "mir/scene/session.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface_creation_parameters.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::GenericShell::GenericShell(
    std::shared_ptr<InputTargeter> const& input_targeter,
    std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<ms::PromptSessionManager> const& prompt_session_manager,
    std::function<std::shared_ptr<shell::WindowManager>(FocusController* focus_controller)> const& wm_builder) :
    AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager, wm_builder)
{
}
