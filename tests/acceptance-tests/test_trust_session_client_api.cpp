/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_toolkit/mir_trust_session.h"
#include "mir/scene/trust_session_listener.h"
#include "mir/scene/trust_session.h"
#include "mir/scene/session.h"
#include "mir/frontend/shell.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace mf = mir::frontend;

using namespace testing;

namespace
{
struct MockTrustSessionListener : ms::TrustSessionListener
{
    MockTrustSessionListener(std::shared_ptr<ms::TrustSessionListener> const& wrapped) :
        wrapped(wrapped)
    {
        ON_CALL(*this, starting(_)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::starting));
        ON_CALL(*this, stopping(_)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::stopping));
        ON_CALL(*this, participant_added(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::participant_added));
        ON_CALL(*this, participant_removed(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::participant_removed));
    }

    MOCK_METHOD1(starting, void(std::shared_ptr<ms::TrustSession> const& trust_session));
    MOCK_METHOD1(stopping, void(std::shared_ptr<ms::TrustSession> const& trust_session));

    MOCK_METHOD2(participant_added, void(ms::TrustSession const& trust_session, std::shared_ptr<ms::Session> const& participant));
    MOCK_METHOD2(participant_removed, void(ms::TrustSession const& trust_session, std::shared_ptr<ms::Session> const& participant));

    std::shared_ptr<ms::TrustSessionListener> const wrapped;
};

struct TrustSessionListenerConfiguration : mtf::StubbedServerConfiguration
{
    std::shared_ptr<ms::TrustSessionListener> the_trust_session_listener()
    {
        return trust_session_listener([this]()
           ->std::shared_ptr<ms::TrustSessionListener>
           {
               return the_mock_trust_session_listener();
           });
    }

    std::shared_ptr<MockTrustSessionListener> the_mock_trust_session_listener()
    {
        return mock_trust_session_listener([this]
            {
                return std::make_shared<NiceMock<MockTrustSessionListener>>(
                    mtf::StubbedServerConfiguration::the_trust_session_listener());
            });
    }

    mir::CachedPtr<MockTrustSessionListener> mock_trust_session_listener;
};

struct TrustSessionClientAPI : mtf::BasicClientServerFixture<TrustSessionListenerConfiguration>
{
    static constexpr int arbitrary_base_session_id = __LINE__;
    static constexpr mir_trust_session_event_callback null_event_callback = nullptr;
    MirTrustSession* trust_session = nullptr;

    MOCK_METHOD2(trust_session_event, void(MirTrustSession* trusted_session, MirTrustSessionState state));
};

extern "C" void trust_session_event_callback(MirTrustSession* trusted_session, MirTrustSessionState state, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->trust_session_event(trusted_session, state);
}

}

TEST_F(TrustSessionClientAPI, can_start_and_stop_a_trust_session)
{
    {
        InSequence server_seq;
        EXPECT_CALL(*server_configuration.the_mock_trust_session_listener(), starting(_));
        EXPECT_CALL(*server_configuration.the_mock_trust_session_listener(), stopping(_));
    }

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);
    ASSERT_THAT(trust_session, Ne(nullptr));

    mir_trust_session_release_sync(trust_session);
}

TEST_F(TrustSessionClientAPI, notifies_start_and_stop)
{
    InSequence seq;
    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_started));
    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_stopped));

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, trust_session_event_callback, this);

    mir_trust_session_release_sync(trust_session);
}

TEST_F(TrustSessionClientAPI, can_add_trusted_session)
{
    pid_t participant_pid = __LINE__;
    auto const participant_session = server_config().the_frontend_shell()->open_session(participant_pid, __PRETTY_FUNCTION__,  std::shared_ptr<mf::EventSink>());

    {
        auto const participant = std::dynamic_pointer_cast<ms::Session>(participant_session);
        InSequence server_seq;
        EXPECT_CALL(*server_configuration.the_mock_trust_session_listener(), participant_added(_, Eq(participant)));
        EXPECT_CALL(*server_configuration.the_mock_trust_session_listener(), participant_removed(_, Eq(participant)));
    }

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);

    EXPECT_TRUE(mir_trust_session_add_trusted_session_sync(trust_session, participant_pid));

    mir_trust_session_release_sync(trust_session);

    // TODO It really shouldn't be necessary to close the participant session.
    // TODO But the MediatingDisplayChanger id destroyed without deregistering
    // TODO callbacks from the BroadcastingSessionEventSink which gets called in
    // TODO SessionManager::~SessionManager() in code that the comments claim
    // TODO works around broken ownership.
    server_config().the_frontend_shell()->close_session(participant_session);
}
