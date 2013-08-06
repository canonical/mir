/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/shell/session_manager.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_container.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/session_listener.h"

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::SessionManager::SessionManager(std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
    std::shared_ptr<msh::SessionContainer> const& container,
    std::shared_ptr<msh::FocusSetter> const& focus_setter,
    std::shared_ptr<msh::SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<msh::SessionListener> const& session_listener) :
    surface_factory(surface_factory),
    app_container(container),
    focus_setter(focus_setter),
    snapshot_strategy(snapshot_strategy),
    session_listener(session_listener)
{
    assert(surface_factory);
    assert(container);
    assert(focus_setter);
    assert(session_listener);
}

msh::SessionManager::~SessionManager()
{
    /*
     * Close all open sessions. We need to do this manually here
     * to break the cyclic dependency between msh::Session
     * and mi::*, since our implementations
     * of these interfaces keep strong references to each other.
     * TODO: Investigate other solutions (e.g. weak_ptr)
     */
    std::vector<std::shared_ptr<msh::Session>> sessions;

    app_container->for_each([&](std::shared_ptr<Session> const& session)
    {
        sessions.push_back(session);
    });

    for (auto& session : sessions)
        close_session(session);
}

std::shared_ptr<mf::Session> msh::SessionManager::open_session(std::string const& name,
                                                std::shared_ptr<mf::EventSink> const& sender)
{
    std::shared_ptr<msh::Session> new_session =
        std::make_shared<msh::ApplicationSession>(
            surface_factory, name, snapshot_strategy, session_listener, sender);

    app_container->insert_session(new_session);
    
    session_listener->starting(new_session);

    focus_setter->set_focus_to(new_session);

    return new_session;
}


void msh::SessionManager::close_session(std::shared_ptr<mf::Session> const& session)
{
    auto shell_session = std::dynamic_pointer_cast<Session>(session);

    session_listener->stopping(shell_session);

    app_container->remove_session(shell_session);

    focus_setter->focus_default();
}

mf::SurfaceId msh::SessionManager::create_surface_for(std::shared_ptr<mf::Session> const& session,
    msh::SurfaceCreationParameters const& params)
{
    auto shell_session = std::dynamic_pointer_cast<Session>(session);
    auto id = shell_session->create_surface(params);

    focus_setter->set_focus_to(shell_session);

    return id;
}
