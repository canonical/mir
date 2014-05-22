/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "trust_session_manager_impl.h"

#include "mir/scene/trust_session_creation_parameters.h"
#include "mir/scene/trust_session_listener.h"
#include "mir/scene/session.h"
#include "session_container.h"
#include "trust_session_impl.h"
#include "trust_session_container.h"

namespace ms = mir::scene;

ms::TrustSessionManagerImpl::TrustSessionManagerImpl(
    std::shared_ptr<SessionContainer> const& app_container,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    trust_session_container(std::make_shared<TrustSessionContainer>()),
    trust_session_listener(trust_session_listener),
    app_container(app_container)
{
}

ms::TrustSessionManagerImpl::~TrustSessionManagerImpl() noexcept
{
}

void ms::TrustSessionManagerImpl::stop_trust_session_locked(
    std::lock_guard<std::mutex> const&,
    std::shared_ptr<TrustSession> const& trust_session) const
{
    trust_session->stop();

    std::vector<std::shared_ptr<Session>> participants;

    trust_session_container->for_each_participant_in_trust_session(trust_session.get(),
        [&](std::weak_ptr<Session> const& session, TrustSessionContainer::TrustType type)
        {
            if (type == TrustSessionContainer::TrustedSession)
                if (auto locked_session = session.lock())
                    participants.push_back(locked_session);
        });

    for (auto session : participants)
    {
        if (trust_session_container->remove_participant(trust_session.get(), session, TrustSessionContainer::TrustedSession))
            trust_session_listener->trusted_session_ending(*trust_session, session);
    }

    trust_session_container->remove_trust_session(trust_session);

    trust_session_listener->stopping(trust_session);
}

void ms::TrustSessionManagerImpl::remove_session(std::shared_ptr<Session> const& session) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    std::vector<std::shared_ptr<TrustSession>> trust_sessions;

    trust_session_container->for_each_trust_session_with_participant(session,
        [&](std::shared_ptr<TrustSession> const& trust_session)
        {
            trust_sessions.push_back(trust_session);
        });

    for(auto trust_session : trust_sessions)
    {
        if (trust_session->get_trusted_helper().lock() == session)
        {
            stop_trust_session_locked(lock, trust_session);
        }
        else
        {
            if (trust_session_container->remove_participant(trust_session.get(), session, TrustSessionContainer::TrustedSession))
                trust_session_listener->trusted_session_ending(*trust_session, session);
        }
    }
}

void ms::TrustSessionManagerImpl::stop_trust_session(std::shared_ptr<TrustSession> const& trust_session) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    stop_trust_session_locked(lock, trust_session);
}

void ms::TrustSessionManagerImpl::add_trusted_process_for_locked(std::lock_guard<std::mutex> const&,
    std::shared_ptr<TrustSession> const& trust_session,
    pid_t process_id) const
{
    trust_session_container->insert_waiting_process(trust_session.get(), process_id);

    app_container->for_each(
        [&](std::shared_ptr<Session> const& session)
        {
            if (session->process_id() == process_id)
            {
                if (trust_session_container->insert_participant(trust_session.get(), session, TrustSessionContainer::TrustedSession))
                    trust_session_listener->trusted_session_beginning(*trust_session, session);
            }
        });
}

void ms::TrustSessionManagerImpl::add_trusted_process_for(
    std::shared_ptr<TrustSession> const& trust_session,
    pid_t process_id) const
{
    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    add_trusted_process_for_locked(lock, trust_session, process_id);
}

std::shared_ptr<ms::TrustSession> ms::TrustSessionManagerImpl::start_trust_session_for(
    std::shared_ptr<Session> const& session,
    TrustSessionCreationParameters const& params) const
{
    auto trust_session = std::make_shared<TrustSessionImpl>(session, params, trust_session_listener);

    std::lock_guard<std::mutex> lock(trust_sessions_mutex);

    trust_session_container->insert_trust_session(trust_session);
    trust_session_container->insert_participant(trust_session.get(), session, TrustSessionContainer::HelperSession);

    trust_session_listener->starting(trust_session);

    add_trusted_process_for_locked(lock, trust_session, params.base_process_id);

    return trust_session;
}

void ms::TrustSessionManagerImpl::add_expected_session(std::shared_ptr<Session> const& session) const
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    std::vector<std::shared_ptr<TrustSession>> trust_sessions;

    trust_session_container->for_each_trust_session_expecting_process(session->process_id(),
        [&](std::shared_ptr<TrustSession> const& trust_session)
        {
            trust_sessions.push_back(trust_session);
        });

    for(auto trust_session : trust_sessions)
    {
        if (trust_session_container->insert_participant(trust_session.get(), session, TrustSessionContainer::TrustedSession))
            trust_session_listener->trusted_session_beginning(*trust_session, session);
    }
}

void ms::TrustSessionManagerImpl::add_trusted_session_for(
    std::shared_ptr<TrustSession> const& trust_session,
    std::shared_ptr<Session> const& session) const
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    if (trust_session_container->insert_participant(trust_session.get(), session, TrustSessionContainer::TrustedSession))
        trust_session_listener->trusted_session_beginning(*trust_session, session);
}

void ms::TrustSessionManagerImpl::for_each_participant_in_trust_session(
    std::shared_ptr<TrustSession> const& trust_session,
    std::function<void(std::shared_ptr<Session> const& participant)> const& f) const
{
    std::unique_lock<std::mutex> lock(trust_sessions_mutex);

    trust_session_container->for_each_participant_in_trust_session(trust_session.get(),
        [&](std::weak_ptr<Session> const& session, TrustSessionContainer::TrustType type)
        {
            if (type == TrustSessionContainer::TrustedSession)
                if (auto locked_session = session.lock())
                    f(locked_session);
        });
}
