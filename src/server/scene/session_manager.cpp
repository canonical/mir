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
#include "session_container.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/shell/focus_setter.h"
#include "mir/scene/session.h"
#include "mir/scene/session_listener.h"
#include "session_event_sink.h"
#include "mir/scene/trust_session_creation_parameters.h"
#include "trust_session_impl.h"
#include "trust_session_container.h"
#include "mir/scene/trust_session_listener.h"

#include <boost/throw_exception.hpp>

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

ms::SessionManager::SessionManager(std::shared_ptr<SurfaceCoordinator> const& surface_factory,
    std::shared_ptr<SessionContainer> const& container,
    std::shared_ptr<msh::FocusSetter> const& focus_setter,
    std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
    std::shared_ptr<SessionEventSink> const& session_event_sink,
    std::shared_ptr<SessionListener> const& session_listener,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    surface_coordinator(surface_factory),
    app_container(container),
    focus_setter(focus_setter),
    snapshot_strategy(snapshot_strategy),
    session_event_sink(session_event_sink),
    session_listener(session_listener),
    trust_session_listener(trust_session_listener),
    trust_session_container(std::make_shared<TrustSessionContainer>())
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
    std::vector<std::shared_ptr<Session>> sessions;

    app_container->for_each([&](std::shared_ptr<Session> const& session)
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
    std::shared_ptr<Session> new_session =
        std::make_shared<ApplicationSession>(
            surface_coordinator, client_pid, name, snapshot_strategy, session_listener, sender);

    app_container->insert_session(new_session);

    session_listener->starting(new_session);

    {
        trust_session_container->for_each_trust_session_for_process(client_pid,
            [client_pid, new_session](std::shared_ptr<frontend::TrustSession> const& trust_session)
            {
                auto scene_trust_session = std::dynamic_pointer_cast<ms::TrustSession>(trust_session);

                scene_trust_session->add_trusted_child(new_session);
            });
    }

    set_focus_to(new_session);

    return new_session;
}

inline void ms::SessionManager::set_focus_to_locked(std::unique_lock<std::mutex> const&, std::shared_ptr<Session> const& session)
{
    auto old_focus = focus_application.lock();

    focus_application = session;

    focus_setter->set_focus_to(session);
    if (session)
    {
        session_event_sink->handle_focus_change(session);
        session_listener->focused(session);
    }
    else
    {
        session_event_sink->handle_no_focus();
        session_listener->unfocused();
    }
}

void ms::SessionManager::set_focus_to(std::shared_ptr<Session> const& session)
{
    std::unique_lock<std::mutex> lg(mutex);
    set_focus_to_locked(lg, session);
}

void ms::SessionManager::close_session(std::shared_ptr<mf::Session> const& session)
{
    auto scene_session = std::dynamic_pointer_cast<Session>(session);

    scene_session->force_requests_to_complete();

    session_event_sink->handle_session_stopping(scene_session);

    {
        std::unique_lock<std::mutex> lock(trust_sessions_mutex);

        std::vector<std::shared_ptr<frontend::TrustSession>> trust_sessions;
        trust_session_container->for_each_trust_session_for_process(scene_session->process_id(),
            [&](std::shared_ptr<frontend::TrustSession> const& trust_session)
            {
                trust_sessions.push_back(trust_session);
            });

        for(auto trust_session : trust_sessions)
        {
            auto scene_trust_session = std::dynamic_pointer_cast<ms::TrustSession>(trust_session);

            if (scene_trust_session->get_trusted_helper().lock() == scene_session)
            {
                stop_trust_session_locked(lock, scene_trust_session);
            }
            else
            {
                scene_trust_session->remove_trusted_child(scene_session);

                trust_session_container->remove_process(scene_session->process_id());
            }
        }
    }

    session_listener->stopping(scene_session);

    app_container->remove_session(scene_session);

    std::unique_lock<std::mutex> lock(mutex);
    auto old_focus = focus_application.lock();
    if (old_focus == scene_session)
    {
        // only reset the focus if this session had focus
        set_focus_to_locked(lock, app_container->successor_of(std::shared_ptr<ms::Session>()));
    }
}

void ms::SessionManager::focus_next()
{
    std::unique_lock<std::mutex> lock(mutex);
    auto focus = focus_application.lock();
    if (!focus)
    {
        focus = app_container->successor_of(std::shared_ptr<Session>());
    }
    else
    {
        focus = app_container->successor_of(focus);
    }
    set_focus_to_locked(lock, focus);
}

std::weak_ptr<ms::Session> ms::SessionManager::focussed_application() const
{
    return focus_application;
}

// TODO: We use this to work around the lack of a SessionMediator-like object for internal clients.
// we could have an internal client mediator which acts as a factory for internal clients, taking responsibility
// for invoking handle_surface_created.
mf::SurfaceId ms::SessionManager::create_surface_for(
    std::shared_ptr<mf::Session> const& session,
    SurfaceCreationParameters const& params)
{
    auto scene_session = std::dynamic_pointer_cast<Session>(session);
    auto id = scene_session->create_surface(params);

    handle_surface_created(session);

    return id;
}

void ms::SessionManager::handle_surface_created(std::shared_ptr<mf::Session> const& session)
{
    set_focus_to(std::dynamic_pointer_cast<Session>(session));
}

std::shared_ptr<mf::TrustSession> ms::SessionManager::start_trust_session_for(std::shared_ptr<mf::Session> const& session,
    TrustSessionCreationParameters const& params)
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    auto shell_session = std::dynamic_pointer_cast<ms::Session>(session);

    auto const trust_session = std::make_shared<TrustSessionImpl>(shell_session, params, trust_session_listener);

    trust_session_container->insert(trust_session, shell_session->process_id());

    trust_session->start();
    trust_session_listener->starting(*(trust_session.get()));

    add_trusted_session_for_locked(lock, trust_session, params.base_process_id);
    return trust_session;
}

MirTrustSessionAddTrustResult ms::SessionManager::add_trusted_session_for(std::shared_ptr<mf::TrustSession> const& trust_session,
    pid_t session_pid)
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    return add_trusted_session_for_locked(lock, trust_session, session_pid);
}

MirTrustSessionAddTrustResult ms::SessionManager::add_trusted_session_for_locked(std::unique_lock<std::mutex> const&,
    std::shared_ptr<mf::TrustSession> const& trust_session,
    pid_t session_pid)
{
    auto scene_trust_session = std::dynamic_pointer_cast<ms::TrustSession>(trust_session);

    if (!trust_session_container->insert(trust_session, session_pid))
    {
        // TODO - remove comment once we have pid update for child fd created sessions.
        // BOOST_THROW_EXCEPTION(std::runtime_error("failed to add trusted session"));
    }

    app_container->for_each(
        [&](std::shared_ptr<ms::Session> const& container_session)
        {
            if (container_session->process_id() == session_pid)
            {
                scene_trust_session->add_trusted_child(container_session);
            }
        });

    return mir_trust_session_add_tust_succeeded;
}

void ms::SessionManager::stop_trust_session(std::shared_ptr<mf::TrustSession> const& trust_session)
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    stop_trust_session_locked(lock, trust_session);
}

void ms::SessionManager::stop_trust_session_locked(std::unique_lock<std::mutex> const&,
    std::shared_ptr<mf::TrustSession> const& trust_session)
{
    auto scene_trust_session = std::dynamic_pointer_cast<ms::TrustSession>(trust_session);

    trust_session->stop();

    trust_session_container->remove_trust_session(trust_session);

    trust_session_listener->stopping(*(scene_trust_session).get());
}

