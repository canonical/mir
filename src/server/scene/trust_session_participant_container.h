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

#ifndef MIR_SCENE_TRUST_SESSION_PARTICIPANT_CONTAINER_H_
#define MIR_SCENE_TRUST_SESSION_PARTICIPANT_CONTAINER_H_

#include <sys/types.h>
#include <mutex>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace mir
{
namespace frontend
{
class TrustSession;
class Session;
}

using boost::multi_index_container;
using namespace boost::multi_index;

namespace scene
{

class TrustSessionParticipantContainer
{
public:
    TrustSessionParticipantContainer();
    virtual ~TrustSessionParticipantContainer() = default;

    bool insert_participant(std::shared_ptr<frontend::TrustSession> const& trust_session, frontend::Session* session);

    void for_each_participant_for_trust_session(std::shared_ptr<frontend::TrustSession> const& trust_session, std::function<void(frontend::Session*)> f);
    void for_each_trust_session_for_participant(frontend::Session* session, std::function<void(std::shared_ptr<frontend::TrustSession> const&)> f);

    void remove_trust_session(std::shared_ptr<frontend::TrustSession> const& trust_session);
    void remove_participant(frontend::Session* session);

    void insert_waiting_process(std::shared_ptr<frontend::TrustSession> const& trust_session, pid_t process_id);
    void for_each_trust_session_for_waiting_process(pid_t process_id, std::function<void(std::shared_ptr<frontend::TrustSession> const&)> f);

private:
    std::mutex mutable mutex;

    typedef struct {
        std::shared_ptr<frontend::TrustSession> trust_session;
        frontend::Session* session;
        uint insert_order;
    } Participant;

    typedef multi_index_container<
        Participant,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    Participant,
                    member<Participant, std::shared_ptr<frontend::TrustSession>, &Participant::trust_session>,
                    member<Participant, uint, &Participant::insert_order>
                >
            >,
            ordered_unique<
                composite_key<
                    Participant,
                    member<Participant, frontend::Session*, &Participant::session>,
                    member<Participant, std::shared_ptr<frontend::TrustSession>, &Participant::trust_session>
                >
            >
        >
    > TrustSessionParticipants;

    typedef nth_index<TrustSessionParticipants,0>::type participant_by_trust_session;
    typedef nth_index<TrustSessionParticipants,1>::type participant_by_session;


    TrustSessionParticipants participant_map;
    participant_by_trust_session& trust_session_index;
    participant_by_session& participant_index;
    static uint insertion_order;



    typedef struct {
        std::shared_ptr<frontend::TrustSession> trust_session;
        pid_t process_id;
    } WaitingProcess;

    typedef multi_index_container<
        WaitingProcess,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    WaitingProcess,
                    member<WaitingProcess, std::shared_ptr<frontend::TrustSession>, &WaitingProcess::trust_session>,
                    member<WaitingProcess, pid_t, &WaitingProcess::process_id>
                >
            >,
            ordered_non_unique<
                member<WaitingProcess, pid_t, &WaitingProcess::process_id>
            >
        >
    > WaitingTrustSessionsProcesses;

    typedef nth_index<WaitingTrustSessionsProcesses,0>::type process_by_trust_session;
    typedef nth_index<WaitingTrustSessionsProcesses,1>::type trust_session_by_process;


    WaitingTrustSessionsProcesses waiting_process_map;
    process_by_trust_session& waiting_process_trust_session_index;
    trust_session_by_process& waiting_process_index;
};

}
}

#endif // MIR_SCENE_TRUST_SESSION_PARTICIPANT_CONTAINER_H_
