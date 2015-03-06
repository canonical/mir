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

#include "mir/shell/focus_controller.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::DefaultWindowManager::DefaultWindowManager(FocusController* focus_controller) :
    focus_controller{focus_controller}
{
}

void msh::DefaultWindowManager::add_session(std::shared_ptr<scene::Session> const& session)
{
    focus_controller->set_focus_to(session, {});
}

void msh::DefaultWindowManager::remove_session(std::shared_ptr<scene::Session> const& /*session*/)
{
}

auto msh::DefaultWindowManager::add_surface(
    std::shared_ptr<scene::Session> const& session,
    scene::SurfaceCreationParameters const& params,
    std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build)
-> frontend::SurfaceId
{
    (void)focus_controller; // SHUT UP CLANG!!
    return build(session, params);
}

void msh::DefaultWindowManager::remove_surface(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::weak_ptr<scene::Surface> const& /*surface*/)
{
}

void msh::DefaultWindowManager::add_display(geometry::Rectangle const& /*area*/)
{
}

void msh::DefaultWindowManager::remove_display(geometry::Rectangle const& /*area*/)
{
}

bool msh::DefaultWindowManager::handle_key_event(MirKeyInputEvent const* /*event*/)
{
    return false;
}

bool msh::DefaultWindowManager::handle_touch_event(MirTouchInputEvent const* /*event*/)
{
    return false;
}

bool msh::DefaultWindowManager::handle_pointer_event(MirPointerInputEvent const* /*event*/)
{
    return false;
}

int msh::DefaultWindowManager::handle_set_state(std::shared_ptr<scene::Surface> const& /*surface*/, MirSurfaceState value)
{
    return value;
}
