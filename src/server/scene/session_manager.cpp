/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "session_manager.h"
#include "application_session.h"
#include "mir/scene/session_container.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/session_listener.h"
#include "session_event_sink.h"
#include "trusted_session_impl.h"

#include <boost/throw_exception.hpp>

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

ms::SessionManager::SessionManager(std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
    std::shared_ptr<SessionContainer> const& container,
    std::shared_ptr<msh::FocusSetter> const& focus_setter,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<SessionEventSink> const& session_event_sink,
    std::shared_ptr<msh::SessionListener> const& session_listener) :
    surface_factory(surface_factory),
    app_container(container),
    focus_setter(focus_setter),
    snapshot_strategy(snapshot_strategy),
    session_event_sink(session_event_sink),
    session_listener(session_listener)
{
    assert(surface_factory);
    assert(container);
    assert(focus_setter);
    assert(session_listener);
}

ms::SessionManager::~SessionManager()
{
    /*
     * Close all open sessions. We need to do this manually here
     * to break the cyclic dependency between msh::Session
     * and mi::*, since our implementations
     * of these interfaces keep strong references to each other.
     * TODO: Investigate other solutions (e.g. weak_ptr)
     */
    std::vector<std::shared_ptr<msh::Session>> sessions;

    app_container->for_each([&](std::shared_ptr<msh::Session> const& session)
    {
        sessions.push_back(session);
    });

    for (auto& session : sessions)
        close_session(session);
}

std::shared_ptr<mf::Session> ms::SessionManager::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sender)
{
    std::shared_ptr<msh::Session> new_session =
        std::make_shared<ApplicationSession>(
            surface_factory, client_pid, name, snapshot_strategy, session_listener, sender);

    app_container->insert_session(new_session);

    {
        std::unique_lock<std::mutex> lock(trusted_sessions_mutex);

        for (auto const& trusted_session : trusted_sessions)
        {
            for (pid_t application_pid : trusted_session.second->get_applications())
            {
                if (application_pid == client_pid)
                {
                    trusted_session.second->add_child_session(new_session);
                    new_session->set_trusted_session(trusted_session.second);
                }
            }
        }
    }

    session_listener->starting(new_session);

    set_focus_to(new_session);

    return new_session;
}

inline void ms::SessionManager::set_focus_to_locked(std::unique_lock<std::mutex> const&, std::shared_ptr<msh::Session> const& shell_session)
{
    auto old_focus = focus_application.lock();

    focus_application = shell_session;

    focus_setter->set_focus_to(shell_session);
    if (shell_session)
    {
        session_event_sink->handle_focus_change(shell_session);
        session_listener->focused(shell_session);
    }
    else
    {
        session_event_sink->handle_no_focus();
        session_listener->unfocused();
    }
}

void ms::SessionManager::set_focus_to(std::shared_ptr<msh::Session> const& shell_session)
{
    std::unique_lock<std::mutex> lg(mutex);
    set_focus_to_locked(lg, shell_session);
}

void ms::SessionManager::close_session(std::shared_ptr<mf::Session> const& session)
{
    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);
    auto scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    session_event_sink->handle_session_stopping(shell_session);

    if (scene_session->get_parent())
    {
        auto scene_session_parent = std::dynamic_pointer_cast<ms::Session>(scene_session->get_parent());
        scene_session->set_parent(NULL);
        scene_session_parent->get_children()->remove_session(shell_session);
    }

    auto trusted_session = shell_session->get_trusted_session();
    if (trusted_session && trusted_session->get_trusted_helper() == shell_session)
    {
        stop_trusted_session(trusted_session->id());
    }

    session_listener->stopping(shell_session);

    app_container->remove_session(shell_session);

    std::unique_lock<std::mutex> lock(mutex);
    auto old_focus = focus_application.lock();
    if (old_focus == shell_session)
    {
        // only reset the focus if this session had focus
        set_focus_to_locked(lock, app_container->successor_of(std::shared_ptr<msh::Session>()));
    }
}

void ms::SessionManager::focus_next()
{
    std::unique_lock<std::mutex> lock(mutex);
    auto focus = focus_application.lock();
    if (!focus)
    {
        focus = app_container->successor_of(std::shared_ptr<msh::Session>());
    }
    else
    {
        focus = app_container->successor_of(focus);
    }
    set_focus_to_locked(lock, focus);
}

std::weak_ptr<msh::Session> ms::SessionManager::focussed_application() const
{
    return focus_application;
}

// TODO: We use this to work around the lack of a SessionMediator-like object for internal clients.
// we could have an internal client mediator which acts as a factory for internal clients, taking responsibility
// for invoking handle_surface_created.
mf::SurfaceId ms::SessionManager::create_surface_for(std::shared_ptr<mf::Session> const& session,
    msh::SurfaceCreationParameters const& params)
{
    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);
    auto id = shell_session->create_surface(params);

    handle_surface_created(session);

    return id;
}

void ms::SessionManager::handle_surface_created(std::shared_ptr<mf::Session> const& session)
{
    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);

    set_focus_to(shell_session);
}

mf::SessionId ms::SessionManager::start_trusted_session_for(std::string& error,
                                            std::shared_ptr<mf::Session> const& session,
                                            shell::TrustedSessionCreationParameters const& params,
                                            std::shared_ptr<mf::EventSink> const& sink)
{
    std::unique_lock<std::mutex> lock(trusted_sessions_mutex);

    for (auto const& trusted_session : trusted_sessions)
    {
        if (trusted_session.second->get_trusted_helper() == session)
        {
            error = "Trusted session already started";
            return mf::SessionId(-1);
        }
    }

    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);
    std::shared_ptr<msh::TrustedSession> trusted_session = TrustedSessionImpl::create_for(shell_session, params, app_container, sink);
    trusted_sessions[trusted_session->id()] = trusted_session;

    session_listener->trusted_session_started(trusted_session);

    return trusted_session->id();
}

void ms::SessionManager::stop_trusted_session(mf::SessionId trusted_session_id)
{
    std::unique_lock<std::mutex> lock(trusted_sessions_mutex);
    auto p = checked_find(trusted_session_id);

    p->second->stop();
    session_listener->trusted_session_stopped(p->second);

    trusted_sessions.erase(p);
}

std::shared_ptr<msh::TrustedSession> ms::SessionManager::get_trusted_session(mf::SessionId id) const
{
    std::unique_lock<std::mutex> lock(trusted_sessions_mutex);

    return checked_find(id)->second;
}

ms::SessionManager::TrustedSessions::const_iterator ms::SessionManager::checked_find(mf::SessionId id) const
{
    auto p = trusted_sessions.find(id);
    if (p == trusted_sessions.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid SessionId"));
    }
    return p;
}
