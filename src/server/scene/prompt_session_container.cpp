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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "prompt_session_container.h"
#include "mir/scene/session.h"

#include <boost/throw_exception.hpp>
#include <atomic>

namespace ms = mir::scene;
namespace mf = mir::frontend;

namespace
{
std::atomic<unsigned> insertion_order{0};
}

ms::PromptSessionContainer::PromptSessionContainer()
    : prompt_session_index(participant_map)
    , participant_index(get<1>(participant_map))
{
}

void ms::PromptSessionContainer::insert_prompt_session(std::shared_ptr<PromptSession> const& prompt_session)
{
    std::unique_lock<std::mutex> lk(mutex);
    prompt_sessions[prompt_session.get()] = prompt_session;
}

void ms::PromptSessionContainer::remove_prompt_session(std::shared_ptr<PromptSession> const& prompt_session)
{
    std::unique_lock<std::mutex> lk(mutex);

    {
        participant_by_prompt_session::iterator it, end;
        boost::tie(it, end) = prompt_session_index.equal_range(prompt_session.get());
        prompt_session_index.erase(it, end);
    }

    prompt_sessions.erase(prompt_session.get());
}

bool ms::PromptSessionContainer::insert_participant(PromptSession* prompt_session, std::weak_ptr<Session> const& session, ParticipantType participant_type)
{
    std::unique_lock<std::mutex> lk(mutex);

    // the prompt session must have first been added by insert_prompt_session.
    if (prompt_sessions.find(prompt_session) == prompt_sessions.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Prompt Session does not exist"));

    if (auto locked_session = session.lock())
    {
        participant_by_prompt_session::iterator it;
        bool valid = false;

        Participant participant{prompt_session, locked_session, participant_type, insertion_order++};
        boost::tie(it,valid) = participant_map.insert(participant);

        return valid;
    }
    return false;
}

bool ms::PromptSessionContainer::remove_participant(PromptSession* prompt_session, std::weak_ptr<Session> const& session, ParticipantType participant_type)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_session::iterator it = participant_index.find(boost::make_tuple(session, participant_type, prompt_session));
    if (it == participant_index.end())
        return false;

    participant_index.erase(it);
    return true;
}

void ms::PromptSessionContainer::for_each_participant_in_prompt_session(
    PromptSession* prompt_session,
    std::function<void(std::weak_ptr<Session> const&, ms::PromptSessionContainer::ParticipantType participant_type)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_prompt_session::iterator it,end;
    boost::tie(it,end) = prompt_session_index.equal_range(prompt_session);

    for (; it != end; ++it)
    {
        Participant const& participant = *it;
        f(participant.session, participant.participant_type);
    }
}

void ms::PromptSessionContainer::for_each_prompt_session_with_participant(
    std::weak_ptr<Session> const& participant,
    ParticipantType participant_type,
    std::function<void(std::shared_ptr<PromptSession> const&)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_session::iterator it,end;
    boost::tie(it,end) = participant_index.equal_range(boost::make_tuple(participant, participant_type));

    for (; it != end; ++it)
    {
        Participant const& participant = *it;

        auto tsit = prompt_sessions.find(participant.prompt_session);
        if (tsit != prompt_sessions.end())
            f(tsit->second);
    }
}

void ms::PromptSessionContainer::for_each_prompt_session_with_participant(
    std::weak_ptr<Session> const& participant,
    std::function<void(std::shared_ptr<PromptSession> const&, ParticipantType)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);
    participant_by_session::iterator it,end;
    boost::tie(it,end) = participant_index.equal_range(participant);

    for (; it != end; ++it)
    {
        Participant const& participant = *it;

        auto tsit = prompt_sessions.find(participant.prompt_session);
        if (tsit != prompt_sessions.end())
            f(tsit->second, participant.participant_type);
    }
}
