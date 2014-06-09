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
#include "mir/scene/trust_session_manager.h"
#include "mir/scene/session.h"
#include "mir/frontend/shell.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test/popen.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <condition_variable>
#include <mutex>

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

    MockTrustSessionListener* the_mock_trust_session_listener()
    {
        return server_configuration.the_mock_trust_session_listener().get();
    }

    MOCK_METHOD2(trust_session_event, void(MirTrustSession* trusted_session, MirTrustSessionState state));

    static std::size_t const arbritary_fd_request_count = 3;
    std::mutex mutex;
    std::size_t actual_fd_count = 0;
    int actual_fds[arbritary_fd_request_count] = {};
    std::condition_variable cv;
    bool called_back = false;

    bool wait_for_callback(std::chrono::milliseconds timeout)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, timeout, [this]{ return called_back; });
    }

    char client_connect_string[128] = {0};

    char const* fd_connect_string(int fd)
    {
        sprintf(client_connect_string, "fd://%d", fd);
        return client_connect_string;
    }

    MOCK_METHOD1(process_line, void(std::string const&));
};

extern "C" void trust_session_event_callback(MirTrustSession* trusted_session, MirTrustSessionState state, void* context)
{
    TrustSessionClientAPI* self = static_cast<TrustSessionClientAPI*>(context);
    self->trust_session_event(trusted_session, state);
}

void client_fd_callback(MirTrustSession*, size_t count, int const* fds, void* context)
{
    auto const self = static_cast<TrustSessionClientAPI*>(context);

    std::unique_lock<decltype(self->mutex)> lock(self->mutex);
    self->actual_fd_count = count;

    std::copy(fds, fds+count, self->actual_fds);
    self->called_back = true;
    self->cv.notify_one();
}

MATCHER_P(SessionWithPid, pid, "")
{
    return arg->process_id() == pid;
}
}

TEST_F(TrustSessionClientAPI, can_start_and_stop_a_trust_session)
{
    {
        InSequence server_seq;
        EXPECT_CALL(*the_mock_trust_session_listener(), starting(_));
        EXPECT_CALL(*the_mock_trust_session_listener(), stopping(_));
    }

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);
    ASSERT_THAT(trust_session, Ne(nullptr));
    EXPECT_THAT(mir_trust_session_get_state(trust_session), Eq(mir_trust_session_state_started));

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
        EXPECT_CALL(*the_mock_trust_session_listener(), participant_added(_, Eq(participant)));
        EXPECT_CALL(*the_mock_trust_session_listener(), participant_removed(_, Eq(participant)));
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

TEST_F(TrustSessionClientAPI, can_get_fds_for_prompt_providers)
{
    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);

    mir_trust_session_new_fds_for_prompt_providers(trust_session, arbritary_fd_request_count, &client_fd_callback, this);
    EXPECT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    EXPECT_THAT(actual_fd_count, Eq(arbritary_fd_request_count));

    mir_trust_session_release_sync(trust_session);
}

TEST_F(TrustSessionClientAPI, when_prompt_provider_connects_over_fd_participant_added_with_right_pid)
{
    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);

    mir_trust_session_new_fds_for_prompt_providers(trust_session, 1, &client_fd_callback, this);
    ASSERT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    auto const expected_pid = getpid();

    EXPECT_CALL(*the_mock_trust_session_listener(), participant_added(_, SessionWithPid(expected_pid)));

    auto client_connection = mir_connect_sync(fd_connect_string(actual_fds[0]), __PRETTY_FUNCTION__);

    mir_connection_release(client_connection);
    mir_trust_session_release_sync(trust_session);
}

// TODO we need a nice way to run this (and similar tests that require a separate client process) in CI
// Disabled as we can't be sure the mir_demo_client_basic is about
TEST_F(TrustSessionClientAPI, DISABLED_client_pid_is_associated_with_session)
{
    auto const server_pid = getpid();

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);

    mir_trust_session_new_fds_for_prompt_providers(trust_session, 1, &client_fd_callback, this);
    wait_for_callback(std::chrono::milliseconds(500));

    EXPECT_CALL(*the_mock_trust_session_listener(), participant_added(_, Not(SessionWithPid(server_pid))));

    InSequence seq;
    EXPECT_CALL(*this, process_line(StrEq("Starting")));
    EXPECT_CALL(*this, process_line(StrEq("Connected")));
    EXPECT_CALL(*this, process_line(StrEq("Surface created")));
    EXPECT_CALL(*this, process_line(StrEq("Surface released")));
    EXPECT_CALL(*this, process_line(StrEq("Connection released")));

    mir::test::Popen output(std::string("bin/mir_demo_client_basic -m ") + fd_connect_string(actual_fds[0]));

    std::string line;
    while (output.get_line(line)) process_line(line);

    mir_trust_session_release_sync(trust_session);
}

TEST_F(TrustSessionClientAPI, notifies_when_server_closes_trust_session)
{
    std::shared_ptr<ms::TrustSession> server_trust_session;

    EXPECT_CALL(*the_mock_trust_session_listener(), starting(_)).
        WillOnce(DoAll(
            Invoke(the_mock_trust_session_listener()->wrapped.get(), &ms::TrustSessionListener::starting),
            SaveArg<0>(&server_trust_session)));
    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_started));

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, trust_session_event_callback, this);

    EXPECT_CALL(*this, trust_session_event(_, mir_trust_session_state_stopped));

    server_configuration.the_trust_session_manager()->stop_trust_session(server_trust_session);

    // Verify we have got the "stopped" notification before we go on and release the session
    Mock::VerifyAndClearExpectations(the_mock_trust_session_listener());

    mir_trust_session_release_sync(trust_session);
}

TEST_F(TrustSessionClientAPI, after_server_closes_trust_session_api_isnt_broken)
{
    std::shared_ptr<ms::TrustSession> server_trust_session;

    EXPECT_CALL(*the_mock_trust_session_listener(), starting(_)).
        WillOnce(DoAll(
            Invoke(the_mock_trust_session_listener()->wrapped.get(), &ms::TrustSessionListener::starting),
            SaveArg<0>(&server_trust_session)));

    MirTrustSession* trust_session = mir_connection_start_trust_session_sync(
        connection, arbitrary_base_session_id, null_event_callback, this);

    server_configuration.the_trust_session_manager()->stop_trust_session(server_trust_session);

    pid_t participant_pid = __LINE__;
    EXPECT_FALSE(mir_trust_session_add_trusted_session_sync(trust_session, participant_pid));
    EXPECT_THAT(mir_trust_session_get_state(trust_session), Eq(mir_trust_session_state_stopped));

    mir_trust_session_release_sync(trust_session);
}
