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
struct StubSceneSession : mtd::StubSceneSession
{
    StubSceneSession(pid_t pid) : pid(pid) {}

    pid_t process_id() const override { return pid; }

    pid_t const pid;
};

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
    pid_t const arbitrary_pid = __LINE__;
    std::shared_ptr<ms::Session> const helper{std::make_shared<StubSceneSession>(helper_pid)};
    std::shared_ptr<ms::Session> const arbitrary_session{std::make_shared<StubSceneSession>(arbitrary_pid)};
    ms::TrustSessionCreationParameters parameters;
    StubSessionContainer existing_sessions;

    NiceMock<mtd::MockTrustSessionListener> trust_session_listener;

    ms::TrustSessionManager session_manager{mt::fake_shared(trust_session_listener)};

    std::shared_ptr<ms::TrustSession> const trust_session{session_manager.start_trust_session_for(helper, parameters, existing_sessions)};
};
}

TEST_F(TrustSessionManager, listener_is_notified_of_trust_session_start_and_stop)
{
    InSequence seq;
    EXPECT_CALL(trust_session_listener, starting(_)).Times(1);

    auto const trust_session = session_manager.start_trust_session_for(helper, parameters, existing_sessions);

    EXPECT_CALL(trust_session_listener, stopping(Eq(trust_session))).Times(1);
    session_manager.stop_trust_session(trust_session);
}

TEST_F(TrustSessionManager, when_session_is_not_in_existing_sessions_listener_is_notified_of_session_beginning)
{
    session_manager.add_trusted_process_for(trust_session, arbitrary_pid, existing_sessions);

    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(arbitrary_session))).Times(1);

    session_manager.add_to_waiting_trust_sessions(arbitrary_session);
}

TEST_F(TrustSessionManager, when_session_is_in_existing_sessions_listener_is_notified_of_session_beginning)
{
    existing_sessions.insert_session(arbitrary_session);

    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(arbitrary_session))).Times(1);

    session_manager.add_trusted_process_for(trust_session, arbitrary_pid, existing_sessions);
}

TEST_F(TrustSessionManager, when_session_added_listener_is_notified_of_session_beginning_and_ending)
{
    session_manager.add_trusted_process_for(trust_session, arbitrary_pid, existing_sessions);

    InSequence seq;
    EXPECT_CALL(trust_session_listener, trusted_session_beginning(Ref(*trust_session), Eq(arbitrary_session))).Times(1);
    EXPECT_CALL(trust_session_listener, trusted_session_ending(Ref(*trust_session), Eq(arbitrary_session))).Times(1);

    session_manager.add_to_waiting_trust_sessions(arbitrary_session);
    session_manager.stop_trust_session(trust_session);
}
