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

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace ms = mir::scene;

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
        ON_CALL(*this, trusted_session_beginning(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::trusted_session_beginning));
        ON_CALL(*this, trusted_session_ending(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::TrustSessionListener::trusted_session_ending));
    }

    MOCK_METHOD1(starting, void(std::shared_ptr<ms::TrustSession> const& trust_session));
    MOCK_METHOD1(stopping, void(std::shared_ptr<ms::TrustSession> const& trust_session));

    MOCK_METHOD2(trusted_session_beginning, void(ms::TrustSession const& trust_session, std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD2(trusted_session_ending, void(ms::TrustSession const& trust_session, std::shared_ptr<ms::Session> const& session));

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
    int started = 0;
    int stopped = 0;

    MOCK_METHOD2(trust_session_event, void(MirTrustSession* trusted_session, MirTrustSessionState state));
};

void trust_session_start_callback(MirTrustSession* session, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->trust_session = session;
    self->started++;
}

void trust_session_stop_callback(MirTrustSession* /* session */, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->stopped++;
}

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

    mir_wait_for(mir_connection_start_trust_session(
        connection, arbitrary_base_session_id, trust_session_start_callback, null_event_callback, this));
    ASSERT_THAT(trust_session, Ne(nullptr));
    EXPECT_THAT(started, Eq(1));

    mir_wait_for(mir_trust_session_stop(trust_session, trust_session_stop_callback, this));
    EXPECT_THAT(stopped, Eq(1));

    mir_trust_session_release(trust_session);
}

TEST_F(TrustSessionClientAPI, notifies_start_and_stop)
{
    InSequence server_seq;
    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_started));
    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_stopped));

    mir_wait_for(mir_connection_start_trust_session(
        connection, arbitrary_base_session_id, trust_session_start_callback, trust_session_event_callback, this));

    mir_wait_for(mir_trust_session_stop(trust_session, trust_session_stop_callback, this));

    mir_trust_session_release(trust_session);
}

// TODO there should be testing with a non-null mir_trust_session_event_callback
// TODO there should be test that clients can be added to a trust session
