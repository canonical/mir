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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/scene/trust_session_manager.h"

#include "src/server/scene/session_container.h"

#include "mir/scene/trust_session.h"
#include "mir/scene/trust_session_creation_parameters.h"

#include "mir_test_doubles/mock_trust_session_listener.h"
#include "mir_test_doubles/stub_scene_session.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
struct StubSessionContainer : ms::SessionContainer
{
    void insert_session(std::shared_ptr<ms::Session> const& session)
    {
        sessions.push_back(session);
    }

    void remove_session(std::shared_ptr<ms::Session> const&)
    {
    }

    void for_each(std::function<void(std::shared_ptr<ms::Session> const&)> f) const
    {
        for (auto const& session : sessions)
            f(session);
    }

    std::shared_ptr<ms::Session> successor_of(std::shared_ptr<ms::Session> const&) const
    {
        return {};
    }

    std::vector<std::shared_ptr<ms::Session>> sessions;
};

struct TrustSessionManager : public testing::Test
{
    pid_t const helper_pid = __LINE__;
    pid_t const participant_pid = __LINE__;
    std::shared_ptr<ms::Session> const helper{std::make_shared<mtd::StubSceneSession>(helper_pid)};
    std::shared_ptr<ms::Session> const participant_session{std::make_shared<mtd::StubSceneSession>(participant_pid)};
    std::shared_ptr<ms::Session> const another_participant{std::make_shared<mtd::StubSceneSession>(__LINE__)};
    ms::TrustSessionCreationParameters parameters;
    StubSessionContainer existing_sessions;

    NiceMock<mtd::MockTrustSessionListener> trust_session_listener;

    ms::TrustSessionManager session_manager{mt::fake_shared(trust_session_listener)};

    std::shared_ptr<ms::TrustSession> const trust_session{session_manager.start_trust_session_for(helper, parameters, existing_sessions)};
};
}

TEST_F(TrustSessionManager, notifies_participant_of_start_and_stop)
{
    InSequence seq;
    EXPECT_CALL(trust_session_listener, starting(_)).Times(1);

    auto const trust_session = session_manager.start_trust_session_for(helper, parameters, existing_sessions);

    EXPECT_CALL(trust_session_listener, stopping(Eq(trust_session))).Times(1);
    session_manager.stop_trust_session(trust_session);

    // Need to verify explicitly as we see unmatched callbacks during teardown of fixture
    Mock::VerifyAndClearExpectations(&trust_session_listener);
}

TEST_F(TrustSessionManager, successfully_adds_a_participant_by_pid)
{
    EXPECT_NO_THROW(session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions));
}

TEST_F(TrustSessionManager, successfully_adds_a_participant)
{
    EXPECT_NO_THROW(session_manager.add_participant(trust_session, participant_session));
}

TEST_F(TrustSessionManager, successfully_adds_a_participant_by_pid_twice)
{
    session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions);
    EXPECT_NO_THROW(session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions));
}

TEST_F(TrustSessionManager, successfully_adds_a_participant_twice)
{
    session_manager.add_participant(trust_session, participant_session);
    EXPECT_NO_THROW(session_manager.add_participant(trust_session, participant_session));
}

TEST_F(TrustSessionManager, fails_to_add_a_participant_by_pid_when_trust_session_is_stopped)
{
    session_manager.stop_trust_session(trust_session);

    EXPECT_THROW(
        session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions),
        std::runtime_error);
}

TEST_F(TrustSessionManager, notifies_session_beginning_when_participant_is_not_in_existing_sessions)
{
    session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions);

    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(participant_session))).Times(1);

    session_manager.add_expected_session(participant_session);
}

TEST_F(TrustSessionManager, notifies_session_beginning_when_participant_is_in_existing_sessions)
{
    existing_sessions.insert_session(participant_session);

    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(participant_session))).Times(1);

    session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions);
}

TEST_F(TrustSessionManager, notifies_session_beginning_and_ending)
{
    session_manager.add_participant_by_pid(trust_session, participant_pid, existing_sessions);

    InSequence seq;
    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(participant_session))).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_ending(Ref(*trust_session), Eq(participant_session))).Times(1);

    session_manager.add_expected_session(participant_session);
    session_manager.stop_trust_session(trust_session);
}

TEST_F(TrustSessionManager, can_iterate_over_participants_in_a_trust_session)
{
    session_manager.add_participant(trust_session, participant_session);
    session_manager.add_participant(trust_session, another_participant);

    struct { MOCK_METHOD1(enumerate, void(std::shared_ptr<ms::Session> const& participant)); } mock;

    EXPECT_CALL(mock, enumerate(participant_session));
    EXPECT_CALL(mock, enumerate(another_participant));

    session_manager.for_each_participant_in_trust_session(
        trust_session,
        [&](std::shared_ptr<ms::Session> const& participant)
            { mock.enumerate(participant); });
}
