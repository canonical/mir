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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_SCENE_PROMPT_SESSION_CONTAINER_H_
#define MIR_SCENE_PROMPT_SESSION_CONTAINER_H_

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
class PromptSession;

class PromptSessionContainer
{
public:
    PromptSessionContainer();
    virtual ~PromptSessionContainer() = default;

    enum class ParticipantType
    {
        helper,
        application,
        prompt_provider,
    };

    void insert_prompt_session(std::shared_ptr<PromptSession> const& prompt_session);
    void remove_prompt_session(std::shared_ptr<PromptSession> const& prompt_session);

    bool insert_participant(PromptSession* prompt_session, std::weak_ptr<Session> const& session, ParticipantType participant_type);
    bool remove_participant(PromptSession* prompt_session, std::weak_ptr<Session> const& session, ParticipantType participant_type);

    void for_each_participant_in_prompt_session(PromptSession* prompt_session, std::function<void(std::weak_ptr<Session> const&, ParticipantType)> f) const;
    void for_each_prompt_session_with_participant(std::weak_ptr<Session> const& participant, ParticipantType participant_type, std::function<void(std::shared_ptr<PromptSession> const&)> f) const;
    void for_each_prompt_session_with_participant(std::weak_ptr<Session> const& participant, std::function<void(std::shared_ptr<PromptSession> const&, ParticipantType)> f) const;

private:
    std::mutex mutable mutex;

    std::unordered_map<PromptSession*, std::shared_ptr<PromptSession>> prompt_sessions;

    struct Participant
    {
        PromptSession* prompt_session;
        std::weak_ptr<Session> session;
        ParticipantType participant_type;
        uint insert_order;
    };

    /**
     * A multi map for associating PromptSessions <-> Sessions.
     * indexed by insertion order for determining the Sessions participating in a PromptSession
     * and indexed for determining the Prompt Sessions in which a Session is participating.
     * A Session can be associated a number of times with a single PromptSession, providing it has a different type
     * eg A Session can be both a helper and a provider for a PromptSession.
     */
    typedef multi_index_container<
        Participant,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    Participant,
                    member<Participant, PromptSession*, &Participant::prompt_session>,
                    member<Participant, uint, &Participant::insert_order>
                >
            >,
            ordered_unique<
                composite_key<
                    Participant,
                    member<Participant, std::weak_ptr<Session>, &Participant::session>,
                    member<Participant, ParticipantType, &Participant::participant_type>,
                    member<Participant, PromptSession*, &Participant::prompt_session>
                >,
                composite_key_compare<
                    std::owner_less<std::weak_ptr<Session>>,
                    std::less<ParticipantType>,
                    std::less<PromptSession*>>
            >
        >
    > PromptSessionParticipants;

    typedef nth_index<PromptSessionParticipants,0>::type participant_by_prompt_session;
    typedef nth_index<PromptSessionParticipants,1>::type participant_by_session;

    PromptSessionParticipants participant_map;
    participant_by_prompt_session& prompt_session_index;
    participant_by_session& participant_index;
};

}
}

#endif // MIR_SCENE_PROMPT_SESSION_CONTAINER_H_
