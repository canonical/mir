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

#include "trust_session_manager.h"

#include "mir/scene/trust_session_creation_parameters.h"
#include "mir/scene/trust_session_listener.h"
#include "mir/scene/session.h"
#include "session_container.h"
#include "trust_session_impl.h"
#include "trust_session_container.h"

namespace ms = mir::scene;

ms::TrustSessionManager::TrustSessionManager(
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    trust_session_container(std::make_shared<TrustSessionContainer>()),
    trust_session_listener(trust_session_listener)
{
}

ms::TrustSessionManager::~TrustSessionManager() noexcept
{
    // TODO this seems unsatisfactory but:
    // TODO TrustSessionImpl has an owning pointer to trust_session_container; and,
    // TODO trust_session_container has an own to TrustSession.
    // TODO Hence we need to clear things to avoid a memory leak

    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    std::vector<std::shared_ptr<TrustSession>> trust_sessions;

    trust_session_container->for_each_trust_session(
        [&](std::shared_ptr<TrustSession> const& trust_session)
        {
            trust_sessions.push_back(trust_session);
        });

    for(auto trust_session : trust_sessions)
    {
        stop_trust_session_locked(lock, trust_session);
    }
}

void ms::TrustSessionManager::stop_trust_session_locked(
    std::lock_guard<std::mutex> const&,
    std::shared_ptr<TrustSession> const& trust_session) const
{
    trust_session->stop();

    trust_session_container->remove_trust_session(trust_session);

    trust_session_listener->stopping(trust_session);
}

void ms::TrustSessionManager::remove_from_trust_sessions(std::shared_ptr<Session> const& session) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    std::vector<std::shared_ptr<TrustSession>> trust_sessions;

    trust_session_container->for_each_trust_session_for_participant(session,
        [&](std::shared_ptr<TrustSession> const& trust_session)
        {
            if (trust_session->get_trusted_helper().lock() == session)
            {
                trust_sessions.push_back(trust_session);
            }
            else
            {
                trust_session->remove_trusted_participant(session);
            }
        });

    for(auto trust_session : trust_sessions)
    {
        stop_trust_session_locked(lock, trust_session);
    }
}

void ms::TrustSessionManager::stop_trust_session(std::shared_ptr<TrustSession> const& trust_session) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);
    stop_trust_session_locked(lock, trust_session);
}

MirTrustSessionAddTrustResult ms::TrustSessionManager::add_trusted_process_for_locked(std::lock_guard<std::mutex> const&,
    std::shared_ptr<TrustSession> const& trust_session,
    pid_t process_id,
    SessionContainer const& existing_session) const
{
    trust_session_container->insert_waiting_process(trust_session.get(), process_id);

    existing_session.for_each(
        [&](std::shared_ptr<Session> const& container_session)
        {
            if (container_session->process_id() == process_id)
            {
                trust_session->add_trusted_participant(container_session);
            }
        });

    return mir_trust_session_add_tust_succeeded;
}

MirTrustSessionAddTrustResult ms::TrustSessionManager::add_trusted_process_for(
    std::shared_ptr<TrustSession> const& trust_session,
    pid_t process_id,
    SessionContainer const& existing_session) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    return add_trusted_process_for_locked(lock, trust_session, process_id, existing_session);
}

std::shared_ptr<ms::TrustSession> ms::TrustSessionManager::start_trust_session_for(
    std::shared_ptr<Session> const& session,
    TrustSessionCreationParameters const& params,
    SessionContainer const& existing_session) const
{
    auto trust_session = std::make_shared<TrustSessionImpl>(session, params, trust_session_listener, trust_session_container);

    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    trust_session_container->insert_trust_session(trust_session);
    trust_session_container->insert_participant(trust_session.get(), session, TrustSessionContainer::HelperSession);

    trust_session->start();
    trust_session_listener->starting(trust_session);

    add_trusted_process_for_locked(lock, trust_session, params.base_process_id, existing_session);

    return trust_session;
}

void ms::TrustSessionManager::add_to_waiting_trust_sessions(std::shared_ptr<Session> const& new_session) const
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    std::vector<std::shared_ptr<TrustSession>> trust_sessions;

    trust_session_container->for_each_trust_session_for_waiting_process(new_session->process_id(),
        [&](std::shared_ptr<TrustSession> const& trust_session)
        {
            trust_sessions.push_back(trust_session);
        });

    for(auto trust_session : trust_sessions)
    {
        trust_session->add_trusted_participant(new_session);
    }
}
