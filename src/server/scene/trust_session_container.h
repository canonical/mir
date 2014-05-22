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

#ifndef MIR_SCENE_TRUST_SESSION_CONTAINER_H_
#define MIR_SCENE_TRUST_SESSION_CONTAINER_H_

#include <sys/types.h>
#include <mutex>
#include <unordered_map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace mir
{
using boost::multi_index_container;
using namespace boost::multi_index;

namespace scene
{
class Session;
class TrustSession;

class TrustSessionContainer
{
public:
    TrustSessionContainer();
    virtual ~TrustSessionContainer() = default;

    typedef enum {
        HelperSession,
        TrustedSession,
    } TrustType;

    void insert_trust_session(std::shared_ptr<TrustSession> const& trust_session);
    void remove_trust_session(std::shared_ptr<TrustSession> const& trust_session);

    bool insert_participant(TrustSession* trust_session, std::weak_ptr<Session> const& session, TrustType trust_type);
    bool remove_participant(TrustSession* trust_session, std::weak_ptr<Session> const& session, TrustType trust_type);

    void for_each_participant_in_trust_session(TrustSession* trust_session, std::function<void(std::weak_ptr<Session> const&, TrustType)> f) const;
    void for_each_trust_session_with_participant(std::weak_ptr<Session> const& participant, TrustType trust_type, std::function<void(std::shared_ptr<TrustSession> const&)> f) const;
    void for_each_trust_session_with_participant(std::weak_ptr<Session> const& participant, std::function<void(std::shared_ptr<TrustSession> const&)> f) const;

    void insert_waiting_process(TrustSession* trust_session, pid_t process_id);
    void for_each_trust_session_expecting_process(pid_t process_id, std::function<void(std::shared_ptr<TrustSession> const&)> f) const;

private:
    std::mutex mutable mutex;

    std::unordered_map<TrustSession*, std::shared_ptr<TrustSession>> trust_sessions;

    typedef struct {
        TrustSession* trust_session;
        std::weak_ptr<Session> session;
        TrustType trust_type;
        uint insert_order;

        Session* session_fun() const { return session.lock().get(); }
    } Participant;

    typedef multi_index_container<
        Participant,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    Participant,
                    member<Participant, TrustSession*, &Participant::trust_session>,
                    member<Participant, uint, &Participant::insert_order>
                >
            >,
            ordered_unique<
                composite_key<
                    Participant,
                    const_mem_fun<Participant, Session*, &Participant::session_fun>,
                    member<Participant, TrustType, &Participant::trust_type>,
                    member<Participant, TrustSession*, &Participant::trust_session>
                >
            >
        >
    > TrustSessionTrustedParticipants;

    typedef nth_index<TrustSessionTrustedParticipants,0>::type participant_by_trust_session;
    typedef nth_index<TrustSessionTrustedParticipants,1>::type participant_by_session;

    TrustSessionTrustedParticipants participant_map;
    participant_by_trust_session& trust_session_index;
    participant_by_session& participant_index;
    static uint insertion_order;

    typedef struct {
        TrustSession* trust_session;
        pid_t process_id;
    } WaitingProcess;

    typedef multi_index_container<
        WaitingProcess,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    WaitingProcess,
                    member<WaitingProcess, TrustSession*, &WaitingProcess::trust_session>,
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

#endif // MIR_SCENE_TRUST_SESSION_CONTAINER_H_
