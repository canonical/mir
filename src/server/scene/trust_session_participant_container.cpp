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

#include "trust_session_participant_container.h"
#include "mir/frontend/session.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

uint ms::TrustSessionParticipantContainer::insertion_order = 0;

ms::TrustSessionParticipantContainer::TrustSessionParticipantContainer()
    : trust_session_index(participant_map)
    , participant_index(get<1>(participant_map))
    , waiting_process_trust_session_index(waiting_process_map)
    , waiting_process_index(get<1>(waiting_process_map))
{
}

bool ms::TrustSessionParticipantContainer::insert_participant(std::shared_ptr<mf::TrustSession> const& trust_session, mf::Session* session, bool child)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_trust_session::iterator it;
    bool valid = false;

    Participant participant{trust_session, session, child, insertion_order++};
    boost::tie(it,valid) = participant_map.insert(participant);

    if (!valid)
        return false;

    process_by_trust_session::iterator process_it,end;
    boost::tie(process_it,end) = waiting_process_trust_session_index.equal_range(boost::make_tuple(trust_session, session->process_id()));
    if (process_it != end)
    {
        waiting_process_trust_session_index.erase(process_it);
    }
    return true;
}

bool ms::TrustSessionParticipantContainer::remove_participant(std::shared_ptr<mf::TrustSession> const& trust_session, mf::Session* session)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_session::iterator it = participant_index.find(boost::make_tuple(session, trust_session));
    if (it == participant_index.end())
        return false;

    participant_index.erase(it);
    return true;
}

void ms::TrustSessionParticipantContainer::remove_trust_session(std::shared_ptr<mf::TrustSession> const& trust_session)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session);

    trust_session_index.erase(it, end);

    {
        process_by_trust_session::iterator it,end;
        boost::tie(it,end) = waiting_process_trust_session_index.equal_range(trust_session);
        waiting_process_trust_session_index.erase(it,end);
    }
}

void ms::TrustSessionParticipantContainer::for_each_participant_for_trust_session(
    std::shared_ptr<mf::TrustSession> const& trust_session,
    std::function<void(mf::Session*, bool)> f)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session);

    for (; it != end; ++it)
    {
        Participant const& participant = *it;
        f(participant.session, participant.child);
    }
}

void ms::TrustSessionParticipantContainer::for_each_trust_session_for_participant(
    mf::Session* session,
    std::function<void(std::shared_ptr<mf::TrustSession> const&)> f)
{
    std::unique_lock<std::mutex> lk(mutex);

    participant_by_session::iterator it,end;
    boost::tie(it,end) = participant_index.equal_range(session);

    for (; it != end; ++it)
    {
        Participant const& participant = *it;
        f(participant.trust_session);
    }
}

void ms::TrustSessionParticipantContainer::insert_waiting_process(
    std::shared_ptr<mf::TrustSession> const& trust_session,
    pid_t process_id)
{
    std::unique_lock<std::mutex> lk(mutex);

    waiting_process_map.insert(WaitingProcess{trust_session, process_id});
}

void ms::TrustSessionParticipantContainer::for_each_trust_session_for_waiting_process(
    pid_t process_id,
    std::function<void(std::shared_ptr<mf::TrustSession> const&)> f)
{
    std::unique_lock<std::mutex> lk(mutex);

    trust_session_by_process::iterator it,end;
    boost::tie(it,end) = waiting_process_index.equal_range(process_id);

    for (; it != end; ++it)
    {
        WaitingProcess const& waiting_process = *it;
        f(waiting_process.trust_session);
    }
}
