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

#include "default_window_manager.h"

#include "mir/scene/session.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/display_layout.h"
#include "mir/shell/focus_controller.h"
#include "mir/shell/surface_ready_observer.h"
#include "mir/shell/surface_specification.h"

#include "mir_toolkit/client_types.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace me = mir::examples;
namespace geom = mir::geometry;

me::DefaultWindowManager::DefaultWindowManager(
    msh::FocusController* focus_controller,
    std::shared_ptr<shell::DisplayLayout> const& display_layout,
    std::shared_ptr<scene::SessionCoordinator> const& session_coordinator) :
    focus_controller{focus_controller},
    display_layout{display_layout},
    session_coordinator{session_coordinator}
{
}

void me::DefaultWindowManager::add_session(std::shared_ptr<scene::Session> const& session)
{
    focus_controller->set_focus_to(session, {});
}

void me::DefaultWindowManager::remove_session(std::shared_ptr<scene::Session> const& /*session*/)
{
    auto const next_session = session_coordinator->successor_of({});
    if (next_session)
        focus_controller->set_focus_to(next_session, next_session->default_surface());
    else
        focus_controller->set_focus_to(next_session, {});
}

auto me::DefaultWindowManager::add_surface(
    std::shared_ptr<scene::Session> const& session,
    scene::SurfaceCreationParameters const& request_parameters,
    std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build)
-> frontend::SurfaceId
{
    mir::graphics::DisplayConfigurationOutputId const output_id_invalid{
        mir_display_output_id_invalid};
    auto placed_parameters = request_parameters;

    geom::Rectangle rect{request_parameters.top_left, request_parameters.size};

    if (request_parameters.output_id != output_id_invalid)
    {
        display_layout->place_in_output(request_parameters.output_id, rect);
    }

    placed_parameters.top_left = rect.top_left;
    placed_parameters.size = rect.size;

    auto const result = build(session, placed_parameters);
    auto const surface = session->surface(result);

    surface->add_observer(std::make_shared<msh::SurfaceReadyObserver>(
        [this](std::shared_ptr<scene::Session> const& session,
               std::shared_ptr<scene::Surface> const& surface)
            {
                focus_controller->set_focus_to(session, surface);
            },
        session,
        surface));

    return result;
}

void me::DefaultWindowManager::modify_surface(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& surface,
    msh::SurfaceSpecification const& modifications)
{
    if (modifications.name.is_set())
        surface->rename(modifications.name.value());
}

void me::DefaultWindowManager::remove_surface(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::weak_ptr<scene::Surface> const& /*surface*/)
{
}

void me::DefaultWindowManager::add_display(geometry::Rectangle const& /*area*/)
{
}

void me::DefaultWindowManager::remove_display(geometry::Rectangle const& /*area*/)
{
}

bool me::DefaultWindowManager::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool me::DefaultWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool me::DefaultWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

int me::DefaultWindowManager::set_surface_attribute(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    return surface->configure(attrib, value);
}
