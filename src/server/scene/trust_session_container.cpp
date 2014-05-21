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

#include "trust_session_container.h"
#include "mir/scene/session.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

uint ms::TrustSessionContainer::insertion_order = 0;

ms::TrustSessionContainer::TrustSessionContainer()
    : trust_session_index(participant_map)
    , participant_index(get<1>(participant_map))
    , waiting_process_trust_session_index(waiting_process_map)
    , waiting_process_index(get<1>(waiting_process_map))
{
}

void ms::TrustSessionContainer::insert_trust_session(std::shared_ptr<TrustSession> const& trust_session)
{
    std::unique_lock<std::mutex> lk(mutex);
    trust_sessions[trust_session.get()] = trust_session;
}

void ms::TrustSessionContainer::remove_trust_session(std::shared_ptr<TrustSession> const& trust_session)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session.get());

    trust_session_index.erase(it, end);

    {
        process_by_trust_session::iterator it,end;
        boost::tie(it,end) = waiting_process_trust_session_index.equal_range(trust_session.get());
        waiting_process_trust_session_index.erase(it,end);
    }

    trust_sessions.erase(trust_session.get());
}

bool ms::TrustSessionContainer::insert_participant(TrustSession* trust_session, std::weak_ptr<Session> const& session, TrustType trust_type)
{
    std::unique_lock<std::mutex> lk(mutex);

    // the trust session must have first been added by insert_trust_session.
    if (trust_sessions.find(trust_session) == trust_sessions.end())
        return false;

    if (auto locked_session = session.lock())
    {
        participant_by_trust_session::iterator it;
        bool valid = false;

        Participant participant{trust_session, locked_session, trust_type, insertion_order++};
        boost::tie(it,valid) = participant_map.insert(participant);

        if (!valid)
            return false;

        process_by_trust_session::iterator process_it,end;
        boost::tie(process_it,end) = waiting_process_trust_session_index.equal_range(boost::make_tuple(trust_session, locked_session->process_id()));
        if (process_it != end)
            waiting_process_trust_session_index.erase(process_it);
        return true;
    }
    return false;
}

bool ms::TrustSessionContainer::remove_participant(TrustSession* trust_session, std::weak_ptr<Session> const& session, TrustType trust_type)
{
    std::unique_lock<std::mutex> lk(mutex);
    if (auto locked_session = session.lock())
    {
        participant_by_session::iterator it = participant_index.find(boost::make_tuple(locked_session.get(), trust_type, trust_session));
        if (it == participant_index.end())
            return false;

        participant_index.erase(it);
        return true;
    }
    return false;
}

void ms::TrustSessionContainer::for_each_participant_in_trust_session(
    TrustSession* trust_session,
    std::function<void(std::weak_ptr<Session> const&, ms::TrustSessionContainer::TrustType trust_type)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session);

    for (; it != end; ++it)
    {
        Participant const& participant = *it;
        f(participant.session, participant.trust_type);
    }
}

void ms::TrustSessionContainer::for_each_trust_session_with_participant(
    std::weak_ptr<Session> const& participant,
    TrustType trust_type,
    std::function<void(std::shared_ptr<TrustSession> const&)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);
    if (auto locked_session = participant.lock())
    {
        participant_by_session::iterator it,end;
        boost::tie(it,end) = participant_index.equal_range(boost::make_tuple(locked_session.get(), trust_type));

        for (; it != end; ++it)
        {
            Participant const& participant = *it;

            auto tsit = trust_sessions.find(participant.trust_session);
            if (tsit != trust_sessions.end())
                f((*tsit).second);
        }
    }
}

void ms::TrustSessionContainer::for_each_trust_session_with_participant(
    std::weak_ptr<Session> const& participant,
    std::function<void(std::shared_ptr<TrustSession> const&)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);
    if (auto locked_session = participant.lock())
    {
        participant_by_session::iterator it,end;
        boost::tie(it,end) = participant_index.equal_range(locked_session.get());

        for (; it != end; ++it)
        {
            Participant const& participant = *it;

            auto tsit = trust_sessions.find(participant.trust_session);
            if (tsit != trust_sessions.end())
                f((*tsit).second);
        }
    }
}

bool ms::TrustSessionContainer::insert_waiting_process(
    TrustSession* trust_session,
    pid_t process_id)
{
    std::unique_lock<std::mutex> lk(mutex);

    // the trust session must have first been added by insert_trust_session.
    if (trust_sessions.find(trust_session) == trust_sessions.end())
        return false;

    waiting_process_map.insert(WaitingProcess{trust_session, process_id});
    return true;
}

void ms::TrustSessionContainer::for_each_trust_session_expecting_process(
    pid_t process_id,
    std::function<void(std::shared_ptr<TrustSession> const&)> f) const
{
    std::unique_lock<std::mutex> lk(mutex);

    trust_session_by_process::iterator it,end;
    boost::tie(it,end) = waiting_process_index.equal_range(process_id);

    for (; it != end; ++it)
    {
        WaitingProcess const& waiting_process = *it;
        auto tsit = trust_sessions.find(waiting_process.trust_session);
        if (tsit != trust_sessions.end())
            f((*tsit).second);
    }
}

void ms::TrustSessionContainer::for_each_trust_session(std::function<void(std::shared_ptr<TrustSession> const&)> f) const
{
    for (auto const& ts : trust_sessions) f(ts.second);
}
