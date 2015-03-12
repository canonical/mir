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

#include "mir/shell/null_window_manager.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;


void msh::NullWindowManager::add_session(std::shared_ptr<scene::Session> const& /*session*/)
{
}

void msh::NullWindowManager::remove_session(std::shared_ptr<scene::Session> const& /*session*/)
{
}

auto msh::NullWindowManager::add_surface(
    std::shared_ptr<scene::Session> const& session,
    scene::SurfaceCreationParameters const& params,
    std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build)
-> frontend::SurfaceId
{
    return build(session, params);
}

void msh::NullWindowManager::remove_surface(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::weak_ptr<scene::Surface> const& /*surface*/)
{
}

void msh::NullWindowManager::add_display(geometry::Rectangle const& /*area*/)
{
}

void msh::NullWindowManager::remove_display(geometry::Rectangle const& /*area*/)
{
}

bool msh::NullWindowManager::handle_key_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool msh::NullWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool msh::NullWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

int msh::NullWindowManager::set_surface_attribute(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& /*surface*/,
    MirSurfaceAttrib /*attrib*/,
    int value)
{
    return value;
}
