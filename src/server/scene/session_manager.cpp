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
#include "mir/shell/surface_factory.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/session_listener.h"
#include "session_event_sink.h"
#include "trust_session.h"
#include "trust_session_container.h"
#include "mir/shell/trust_session_creation_parameters.h"

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
    session_listener(session_listener),
    trust_session_container(std::make_shared<TrustSessionContainer>())
{
    assert(surface_factory);
    assert(container);
    assert(focus_setter);
    assert(session_listener);


    printf("TESTING\n");

    auto ptr1 = std::make_shared<TrustSession>(std::weak_ptr<msh::Session>(), msh::a_trust_session());
    auto ptr2 = std::make_shared<TrustSession>(std::weak_ptr<msh::Session>(), msh::a_trust_session());
    auto ptr3 = std::make_shared<TrustSession>(std::weak_ptr<msh::Session>(), msh::a_trust_session());
    auto ptr4 = std::make_shared<TrustSession>(std::weak_ptr<msh::Session>(), msh::a_trust_session());

    trust_session_container->insert(ptr2, TrustSessionContainer::ClientProcess{8});
    trust_session_container->insert(ptr2, TrustSessionContainer::ClientProcess{6});
    trust_session_container->insert(ptr2, TrustSessionContainer::ClientProcess{7});
    trust_session_container->insert(ptr2, TrustSessionContainer::ClientProcess{5});

    trust_session_container->insert(ptr1, TrustSessionContainer::ClientProcess{1});
    trust_session_container->insert(ptr1, TrustSessionContainer::ClientProcess{2});
    trust_session_container->insert(ptr1, TrustSessionContainer::ClientProcess{3});
    trust_session_container->insert(ptr1, TrustSessionContainer::ClientProcess{4});
    trust_session_container->insert(ptr1, TrustSessionContainer::ClientProcess{5});

    trust_session_container->insert(ptr3, TrustSessionContainer::ClientProcess{9});
    trust_session_container->insert(ptr3, TrustSessionContainer::ClientProcess{5});

    trust_session_container->insert(ptr4, TrustSessionContainer::ClientProcess{10});
    trust_session_container->insert(ptr4, TrustSessionContainer::ClientProcess{11});
    trust_session_container->insert(ptr4, TrustSessionContainer::ClientProcess{11});
    trust_session_container->insert(ptr4, TrustSessionContainer::ClientProcess{11});

    trust_session_container->remove_trust_session(ptr3);
    trust_session_container->remove_process(TrustSessionContainer::ClientProcess{5});



    printf("\nProcesses for 1:\n");
    trust_session_container->for_each_process_for_trust_session(ptr1,
        [](TrustSessionContainer::ClientProcess const& process)
        {
            printf("   pid %d\n", process);

        });

    printf("\nProcesses for 2:\n");
    trust_session_container->for_each_process_for_trust_session(ptr2,
        [](TrustSessionContainer::ClientProcess const& process)
        {
            printf("   pid %d\n", process);

        });

    printf("\nProcesses for 3:\n");
    trust_session_container->for_each_process_for_trust_session(ptr3,
        [](TrustSessionContainer::ClientProcess const& process)
        {
            printf("   pid %d\n", process);

        });

    printf("\nProcesses for 4:\n");
    trust_session_container->for_each_process_for_trust_session(ptr4,
        [](TrustSessionContainer::ClientProcess const& process)
        {
            printf("   pid %d\n", process);

        });



    printf("\nTrust sessions for process 1:\n");
    trust_session_container->for_each_trust_session_for_process(TrustSessionContainer::ClientProcess{1},
        [](std::shared_ptr<frontend::TrustSession> const& trust_session)
        {
            printf("   trust session %p\n", (void*)trust_session.get());
        });

    printf("\nTrust sessions for process 5:\n");
    trust_session_container->for_each_trust_session_for_process(TrustSessionContainer::ClientProcess{5},
        [](std::shared_ptr<frontend::TrustSession> const& trust_session)
        {
            printf("   trust session %p\n", (void*)trust_session.get());
        });
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
        trust_session_container->for_each_trust_session_for_process(client_pid,
            [client_pid, new_session](std::shared_ptr<frontend::TrustSession> const& trust_session)
            {
                printf("found after %d\n", client_pid);
                auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(trust_session);

                shell_trust_session->add_trusted_child(new_session);
                new_session->begin_trust_session(shell_trust_session);
            });
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

    {
        std::unique_lock<std::mutex> lock(trust_sessions_mutex);

        std::vector<std::shared_ptr<frontend::TrustSession>> trust_sessions;
        trust_session_container->for_each_trust_session_for_process(shell_session->process_id(),
            [&](std::shared_ptr<frontend::TrustSession> const& trust_session)
            {
                trust_sessions.push_back(trust_session);
            });

        for(auto trust_session : trust_sessions)
        {
            auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(trust_session);

            if (shell_trust_session->get_trusted_helper().lock() == shell_session)
            {
                stop_trust_session_locked(lock, shell_trust_session);
            }
            else
            {
                shell_trust_session->remove_trusted_child(shell_session);
                shell_session->end_trust_session(shell_trust_session);

                trust_session_container->remove_process(shell_session->process_id());
            }
        }
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

std::shared_ptr<mf::TrustSession> ms::SessionManager::start_trust_session_for(std::string&,
    std::shared_ptr<mf::Session> const& session,
    shell::TrustSessionCreationParameters const& params)
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    auto shell_session = std::dynamic_pointer_cast<msh::Session>(session);

    auto const trust_session = std::make_shared<TrustSession>(shell_session, params);

    trust_session_container->insert(trust_session, shell_session->process_id());

    trust_session->start();
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
    auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(trust_session);

    trust_session_container->insert(trust_session, session_pid);

    app_container->for_each(
        [this, session_pid, shell_trust_session](std::shared_ptr<msh::Session> const& container_session)
        {
            if (container_session->process_id() == session_pid)
            {
                printf("found during %d\n", session_pid);
                if (shell_trust_session->add_trusted_child(container_session))
                {
                    container_session->begin_trust_session(shell_trust_session);
                }
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
    auto shell_trust_session = std::dynamic_pointer_cast<msh::TrustSession>(trust_session);

    auto trusted_helper = shell_trust_session->get_trusted_helper().lock();
    if (trusted_helper) {
        trusted_helper->end_trust_session(shell_trust_session);
    }

    shell_trust_session->for_each_trusted_child(
        [this, shell_trust_session](std::shared_ptr<msh::Session> const& child_session)
        {
            child_session->end_trust_session(shell_trust_session);
        },
        true
    );

    trust_session->stop();

    trust_session_container->remove_trust_session(trust_session);
}

