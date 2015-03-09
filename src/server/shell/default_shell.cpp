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
#include "mir/shell/input_targeter.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_configurator.h"
#include "mir/scene/surface_coordinator.h"

namespace mf = mir::frontend;
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
        [&](FocusController* focus_controller) { return std::make_shared<DefaultWindowManager>(focus_controller, placement_strategy, session_coordinator); }),
    surface_configurator{surface_configurator}
{
}

int msh::DefaultShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    auto const configured_value = surface_configurator->select_attribute_value(*surface, attrib, value);

    auto const result = AbstractShell::set_surface_attribute(session, surface, attrib, configured_value);

    surface_configurator->attribute_set(*surface, attrib, result);

    return result;
}

void msh::DefaultShell::setting_focus_to(std::shared_ptr<ms::Surface> const& surface)
{
    if (surface)
        surface_coordinator->raise(surface);
}
