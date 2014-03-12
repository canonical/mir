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
#include "trust_session.h"

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
        // Add session to existing trust session
        std::unique_lock<std::mutex> lock(trust_sessions_mutex);

        auto find_trusted_sesssion_fn =
            [client_pid](std::shared_ptr<frontend::TrustSession> const& trust_session)
            {
                auto applications = trust_session->get_applications();
                auto it_apps = std::find(applications.begin(), applications.end(), client_pid);
                return it_apps != applications.end();
            };

        auto it = trust_sessions.begin();
        while((it = std::find_if(it, trust_sessions.end(), find_trusted_sesssion_fn)) != trust_sessions.end())
        {
            auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(*it);

            shell_trust_session->add_child_session(new_session);
            new_session->set_trust_session(shell_trust_session);
            it++;
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

    shell_session->force_requests_to_complete();

    session_event_sink->handle_session_stopping(shell_session);

    auto session_parent = shell_session->get_parent();
    if (session_parent)
    {
        shell_session->set_parent(NULL);
        session_parent->get_children()->remove_session(shell_session);
    }

    auto trust_session = shell_session->get_trust_session();
    if (trust_session && trust_session->get_trusted_helper() == shell_session)
    {
        stop_trust_session(trust_session);
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

std::shared_ptr<mf::TrustSession> ms::SessionManager::start_trust_session_for(std::string& error,
                                            std::shared_ptr<mf::Session> const& session,
                                            shell::TrustSessionCreationParameters const& params,
                                            std::shared_ptr<mf::EventSink> const& sink)
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    auto it = std::find_if(trust_sessions.begin(), trust_sessions.end(),
        [session](std::shared_ptr<frontend::TrustSession> const& trust_session)
        {
            auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(trust_session);

            return shell_trust_session->get_trusted_helper() == session;
        }
    );
    if (it != trust_sessions.end())
    {
        error = "Trust session already started";
        return std::shared_ptr<mf::TrustSession>();
    }

    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);
    std::shared_ptr<msh::TrustSession> trust_session = TrustSession::start_for(shell_session, params, app_container, sink);
    trust_sessions.push_back(trust_session);

    session_listener->trust_session_started(trust_session);

    return trust_session;
}

void ms::SessionManager::stop_trust_session(std::shared_ptr<mf::TrustSession> const& trust_session)
{
    auto it = std::find(trust_sessions.begin(), trust_sessions.end(), trust_session);
    if (it != trust_sessions.end())
    {
        (*it)->stop();
        session_listener->trust_session_stopped(std::dynamic_pointer_cast<msh::TrustSession>(trust_session));

        trust_sessions.erase(it);
    }
}
