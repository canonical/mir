/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "prompt_session_manager_impl.h"

#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/scene/prompt_session_listener.h"
#include "mir/scene/session.h"
#include "mir/scene/session_container.h"
#include "prompt_session_container.h"
#include "prompt_session_impl.h"

namespace ms = mir::scene;

ms::PromptSessionManagerImpl::PromptSessionManagerImpl(
    std::shared_ptr<SessionContainer> const& app_container,
    std::shared_ptr<PromptSessionListener> const& prompt_session_listener) :
    prompt_session_container(std::make_shared<PromptSessionContainer>()),
    prompt_session_listener(prompt_session_listener),
    app_container(app_container)
{
}

void ms::PromptSessionManagerImpl::stop_prompt_session_locked(
    std::lock_guard<std::mutex> const&,
    std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::vector<std::shared_ptr<Session>> participants;

    prompt_session_container->for_each_participant_in_prompt_session(prompt_session.get(),
        [&](std::weak_ptr<Session> const& session, PromptSessionContainer::ParticipantType type)
        {
            if (type == PromptSessionContainer::ParticipantType::prompt_provider)
            {
                if (auto locked_session = session.lock())
                    participants.push_back(locked_session);
            }
        });

    for (auto const& participant : participants)
    {
        if (prompt_session_container->remove_participant(prompt_session.get(), participant, PromptSessionContainer::ParticipantType::prompt_provider))
            prompt_session_listener->prompt_provider_removed(*prompt_session, participant);
    }

    prompt_session->stop(helper_for(prompt_session));

    prompt_session_container->remove_prompt_session(prompt_session);

    prompt_session_listener->stopping(prompt_session);
}

void ms::PromptSessionManagerImpl::remove_session(std::shared_ptr<Session> const& session) const
{
    std::lock_guard<std::mutex> lock(prompt_sessions_mutex);

    std::vector<std::pair<std::shared_ptr<PromptSession>, PromptSessionContainer::ParticipantType>> prompt_sessions;

    prompt_session_container->for_each_prompt_session_with_participant(session,
        [&](std::shared_ptr<PromptSession> const& prompt_session, PromptSessionContainer::ParticipantType participant_type)
        {
            prompt_sessions.push_back(std::make_pair(prompt_session, participant_type));
        });

    for(auto const& prompt_session : prompt_sessions)
    {
        if (prompt_session.second == PromptSessionContainer::ParticipantType::helper)
        {
            stop_prompt_session_locked(lock, prompt_session.first);
        }
        else
        {
            if (prompt_session_container->remove_participant(prompt_session.first.get(), session, prompt_session.second))
            {
                if (prompt_session.second == PromptSessionContainer::ParticipantType::prompt_provider)
                    prompt_session_listener->prompt_provider_removed(*prompt_session.first, session);
            }
        }
    }
}

void ms::PromptSessionManagerImpl::stop_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::lock_guard<std::mutex> lock(prompt_sessions_mutex);

    stop_prompt_session_locked(lock, prompt_session);
}

void ms::PromptSessionManagerImpl::suspend_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::lock_guard<std::mutex> lock(prompt_sessions_mutex);

    prompt_session->suspend(helper_for(prompt_session));
    prompt_session_listener->suspending(prompt_session);
}

void ms::PromptSessionManagerImpl::resume_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::lock_guard<std::mutex> lock(prompt_sessions_mutex);

    prompt_session->resume(helper_for(prompt_session));
    prompt_session_listener->resuming(prompt_session);
}

std::shared_ptr<ms::PromptSession> ms::PromptSessionManagerImpl::start_prompt_session_for(
    std::shared_ptr<Session> const& session,
    PromptSessionCreationParameters const& params) const
{
    auto prompt_session = std::make_shared<PromptSessionImpl>();
    std::shared_ptr<Session> application_session;

    app_container->for_each(
    [&](std::shared_ptr<Session> const& session)
    {
        if (session->process_id() == params.application_pid)
        {
            application_session = session;
        }
    });

    if (!application_session)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not identify application session"));

    std::lock_guard<std::mutex> lock(prompt_sessions_mutex);

    prompt_session_container->insert_prompt_session(prompt_session);
    if (!prompt_session_container->insert_participant(prompt_session.get(), session, PromptSessionContainer::ParticipantType::helper))
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not set prompt session helper"));

    prompt_session->start(session);
    prompt_session_listener->starting(prompt_session);

    prompt_session_container->insert_participant(prompt_session.get(), application_session, PromptSessionContainer::ParticipantType::application);

    return prompt_session;
}

void ms::PromptSessionManagerImpl::add_prompt_provider(
    std::shared_ptr<PromptSession> const& prompt_session,
    std::shared_ptr<Session> const& prompt_provider) const
{
    std::unique_lock<std::mutex> lock(prompt_sessions_mutex);

    if (prompt_session_container->insert_participant(prompt_session.get(), prompt_provider, PromptSessionContainer::ParticipantType::prompt_provider))
        prompt_session_listener->prompt_provider_added(*prompt_session, prompt_provider);
}

std::shared_ptr<ms::Session> ms::PromptSessionManagerImpl::application_for(
    std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::shared_ptr<Session> application_session;

    prompt_session_container->for_each_participant_in_prompt_session(prompt_session.get(),
        [&](std::weak_ptr<Session> const& session, PromptSessionContainer::ParticipantType type)
        {
            if (type == PromptSessionContainer::ParticipantType::application)
                application_session = session.lock();
        });
    return application_session;
}

std::shared_ptr<ms::Session> ms::PromptSessionManagerImpl::helper_for(
    std::shared_ptr<PromptSession> const& prompt_session) const
{
    std::shared_ptr<Session> helper_session;

    prompt_session_container->for_each_participant_in_prompt_session(prompt_session.get(),
        [&](std::weak_ptr<Session> const& session, PromptSessionContainer::ParticipantType type)
        {
            if (type == PromptSessionContainer::ParticipantType::helper)
                helper_session = session.lock();
        });
    return helper_session;
}

void ms::PromptSessionManagerImpl::for_each_provider_in(
    std::shared_ptr<PromptSession> const& prompt_session,
    std::function<void(std::shared_ptr<Session> const& prompt_provider)> const& f) const
{
    prompt_session_container->for_each_participant_in_prompt_session(prompt_session.get(),
        [&](std::weak_ptr<Session> const& session, PromptSessionContainer::ParticipantType type)
        {
            if (type == PromptSessionContainer::ParticipantType::prompt_provider)
                if (auto locked_session = session.lock())
                    f(locked_session);
        });
}
