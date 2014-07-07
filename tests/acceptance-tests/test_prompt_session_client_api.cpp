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

#include "mir_toolkit/mir_prompt_session.h"
#include "mir/scene/prompt_session_listener.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/session.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/shell.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_doubles/stub_session_authorizer.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test/popen.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <condition_variable>
#include <mutex>

namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace mf = mir::frontend;

using namespace testing;

namespace
{
struct MockPromptSessionListener : ms::PromptSessionListener
{
    MockPromptSessionListener(std::shared_ptr<ms::PromptSessionListener> const& wrapped) :
        wrapped(wrapped)
    {
        ON_CALL(*this, starting(_)).WillByDefault(Invoke(wrapped.get(), &ms::PromptSessionListener::starting));
        ON_CALL(*this, stopping(_)).WillByDefault(Invoke(wrapped.get(), &ms::PromptSessionListener::stopping));
        ON_CALL(*this, prompt_provider_added(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::PromptSessionListener::prompt_provider_added));
        ON_CALL(*this, prompt_provider_removed(_, _)).WillByDefault(Invoke(wrapped.get(), &ms::PromptSessionListener::prompt_provider_removed));
    }

    MOCK_METHOD1(starting, void(std::shared_ptr<ms::PromptSession> const& prompt_session));
    MOCK_METHOD1(stopping, void(std::shared_ptr<ms::PromptSession> const& prompt_session));

    MOCK_METHOD2(prompt_provider_added, void(ms::PromptSession const& prompt_session, std::shared_ptr<ms::Session> const& prompt_provider));
    MOCK_METHOD2(prompt_provider_removed, void(ms::PromptSession const& prompt_session, std::shared_ptr<ms::Session> const& prompt_provider));

    std::shared_ptr<ms::PromptSessionListener> const wrapped;
};

struct MockSessionAuthorizer : public mtd::StubSessionAuthorizer
{
    MockSessionAuthorizer()
    {
        ON_CALL(*this, prompt_session_is_allowed(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(prompt_session_is_allowed, bool(mf::SessionCredentials const&));
};

struct PromptSessionTestConfiguration : mtf::StubbedServerConfiguration
{
    std::shared_ptr<ms::PromptSessionListener> the_prompt_session_listener() override
    {
        return prompt_session_listener([this]()
           ->std::shared_ptr<ms::PromptSessionListener>
           {
               return the_mock_prompt_session_listener();
           });
    }

    std::shared_ptr<MockPromptSessionListener> the_mock_prompt_session_listener()
    {
        return mock_prompt_session_listener([this]
            {
                return std::make_shared<NiceMock<MockPromptSessionListener>>(
                    mtf::StubbedServerConfiguration::the_prompt_session_listener());
            });
    }

    std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
    {
        return session_authorizer([this]()
           ->std::shared_ptr<mf::SessionAuthorizer>
           {
               return the_mock_session_authorizer();
           });
    }

    std::shared_ptr<MockSessionAuthorizer> the_mock_session_authorizer()
    {
        return mock_prompt_session_authorizer([this]
            {
                return std::make_shared<NiceMock<MockSessionAuthorizer>>();
            });
    }

    mir::CachedPtr<MockPromptSessionListener> mock_prompt_session_listener;
    mir::CachedPtr<MockSessionAuthorizer> mock_prompt_session_authorizer;
};

using BasicClientServerFixture = mtf::BasicClientServerFixture<PromptSessionTestConfiguration>;

struct PromptSessionClientAPI : BasicClientServerFixture
{
    static constexpr mir_prompt_session_state_change_callback null_state_change_callback = nullptr;

    static constexpr pid_t application_session_pid = __LINE__;
    std::shared_ptr<mf::Session> application_session;

    static constexpr pid_t existing_prompt_provider_pid = __LINE__;
    std::shared_ptr<mf::Session> existing_prompt_provider_session;

    static constexpr pid_t another_prompt_provider_pid = __LINE__;
    std::shared_ptr<mf::Session> another_existing_prompt_provider;

    std::shared_ptr<ms::PromptSession> server_prompt_session;

    void SetUp() override
    {
        BasicClientServerFixture::SetUp();
        application_session = server_config().the_frontend_shell()->open_session(application_session_pid, __PRETTY_FUNCTION__,  std::shared_ptr<mf::EventSink>());
        existing_prompt_provider_session = server_config().the_frontend_shell()->open_session(existing_prompt_provider_pid, __PRETTY_FUNCTION__,  std::shared_ptr<mf::EventSink>());
        another_existing_prompt_provider = server_config().the_frontend_shell()->open_session(another_prompt_provider_pid, __PRETTY_FUNCTION__,  std::shared_ptr<mf::EventSink>());
    }

    void capture_server_prompt_session()
    {
        EXPECT_CALL(*the_mock_prompt_session_listener(), starting(_)).
            WillOnce(DoAll(
                Invoke(the_mock_prompt_session_listener()->wrapped.get(), &ms::PromptSessionListener::starting),
                SaveArg<0>(&server_prompt_session)));
    }

    void TearDown() override
    {
        // TODO It really shouldn't be necessary to close these sessions.
        // TODO But the MediatingDisplayChanger id destroyed without deregistering
        // TODO callbacks from the BroadcastingSessionEventSink which gets called in
        // TODO SessionManager::~SessionManager() in code that the comments claim
        // TODO works around broken ownership.
        server_config().the_frontend_shell()->close_session(another_existing_prompt_provider);
        server_config().the_frontend_shell()->close_session(existing_prompt_provider_session);
        server_config().the_frontend_shell()->close_session(application_session);

        BasicClientServerFixture::TearDown();
    }

    MockPromptSessionListener* the_mock_prompt_session_listener()
    {
        return server_configuration.the_mock_prompt_session_listener().get();
    }

    MockSessionAuthorizer& the_mock_session_authorizer()
    {
        return *server_configuration.the_mock_session_authorizer();
    }

    MOCK_METHOD2(prompt_session_state_change, void(MirPromptSession* prompt_provider, MirPromptSessionState state));

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

    char const* fd_connect_string(int fd)
    {
        static char client_connect_string[32] = {0};

        sprintf(client_connect_string, "fd://%d", fd);
        return client_connect_string;
    }

    MOCK_METHOD1(process_line, void(std::string const&));

    std::vector<std::shared_ptr<mf::Session>> list_providers_for(std::shared_ptr<ms::PromptSession> const& prompt_session)
    {
        std::vector<std::shared_ptr<mf::Session>> results;
        auto providers_fn = [&results](std::weak_ptr<ms::Session> const& session)
        {
            results.push_back(session.lock());
        };

        server_configuration.the_prompt_session_manager()->for_each_provider_in(prompt_session, providers_fn);

        return results;
    }
};

extern "C" void prompt_session_state_change_callback(MirPromptSession* prompt_provider, MirPromptSessionState state, void* context)
{
    PromptSessionClientAPI* self = static_cast<PromptSessionClientAPI*>(context);
    self->prompt_session_state_change(prompt_provider, state);
}

void client_fd_callback(MirPromptSession*, size_t count, int const* fds, void* context)
{
    auto const self = static_cast<PromptSessionClientAPI*>(context);

    std::unique_lock<decltype(self->mutex)> lock(self->mutex);
    self->actual_fd_count = count;

    std::copy(fds, fds+count, self->actual_fds);
    self->called_back = true;
    self->cv.notify_one();
}

struct DummyPromptProvider
{
    DummyPromptProvider(char const* connect_string, char const* app_name) :
        connection{mir_connect_sync(connect_string, app_name)}
    {
        EXPECT_THAT(connection, NotNull());
    }

    ~DummyPromptProvider() noexcept
    {
        mir_connection_release(connection);
    }

    MirConnection* const connection;
};

MATCHER_P(SessionWithPid, pid, "")
{
    return arg->process_id() == pid;
}

MATCHER_P(SessionWithName, name, "")
{
    return arg->name() == name;
}
}

TEST_F(PromptSessionClientAPI, can_start_and_stop_a_prompt_session)
{
    {
        InSequence server_seq;
        EXPECT_CALL(*the_mock_prompt_session_listener(), starting(_));
        EXPECT_CALL(*the_mock_prompt_session_listener(), stopping(_));
    }

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);
    ASSERT_THAT(prompt_session, Ne(nullptr));
    EXPECT_THAT(mir_prompt_session_is_valid(prompt_session), Eq(true));
    EXPECT_THAT(mir_prompt_session_error_message(prompt_session), StrEq(""));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, notifies_start_and_stop)
{
    InSequence seq;
    EXPECT_CALL(*this, prompt_session_state_change(_, mir_prompt_session_state_started));
    EXPECT_CALL(*this, prompt_session_state_change(_, mir_prompt_session_state_stopped));

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, prompt_session_state_change_callback, this);

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, can_add_preexisting_prompt_provider)
{
    {
        auto const prompt_provider = std::dynamic_pointer_cast<ms::Session>(existing_prompt_provider_session);
        InSequence server_seq;
        EXPECT_CALL(*the_mock_prompt_session_listener(), prompt_provider_added(_, Eq(prompt_provider)));
        EXPECT_CALL(*the_mock_prompt_session_listener(), prompt_provider_removed(_, Eq(prompt_provider)));
    }

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    EXPECT_TRUE(mir_prompt_session_add_prompt_provider_sync(prompt_session, existing_prompt_provider_pid));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, can_get_fds_for_prompt_providers)
{
    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    mir_prompt_session_new_fds_for_prompt_providers(prompt_session, arbritary_fd_request_count, &client_fd_callback, this);
    EXPECT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    EXPECT_THAT(actual_fd_count, Eq(arbritary_fd_request_count));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, when_prompt_provider_connects_over_fd_prompt_provider_added_with_right_pid)
{
    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    mir_prompt_session_new_fds_for_prompt_providers(prompt_session, 1, &client_fd_callback, this);
    ASSERT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    auto const expected_pid = getpid();

    EXPECT_CALL(*the_mock_prompt_session_listener(), prompt_provider_added(_, SessionWithPid(expected_pid)));

    DummyPromptProvider{fd_connect_string(actual_fds[0]), __PRETTY_FUNCTION__};

    mir_prompt_session_release_sync(prompt_session);
}

// TODO we need a nice way to run this (and similar tests that require a separate client process) in CI
// Disabled as we can't be sure the mir_demo_client_basic is about
TEST_F(PromptSessionClientAPI, DISABLED_client_pid_is_associated_with_session)
{
    auto const server_pid = getpid();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    mir_prompt_session_new_fds_for_prompt_providers(prompt_session, 1, &client_fd_callback, this);
    wait_for_callback(std::chrono::milliseconds(500));

    EXPECT_CALL(*the_mock_prompt_session_listener(), prompt_provider_added(_, Not(SessionWithPid(server_pid))));

    InSequence seq;
    EXPECT_CALL(*this, process_line(StrEq("Starting")));
    EXPECT_CALL(*this, process_line(StrEq("Connected")));
    EXPECT_CALL(*this, process_line(StrEq("Surface created")));
    EXPECT_CALL(*this, process_line(StrEq("Surface released")));
    EXPECT_CALL(*this, process_line(StrEq("Connection released")));

    mir::test::Popen output(std::string("bin/mir_demo_client_basic -m ") + fd_connect_string(actual_fds[0]));

    std::string line;
    while (output.get_line(line)) process_line(line);

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, notifies_when_server_closes_prompt_session)
{
    EXPECT_CALL(*this, prompt_session_state_change(_, mir_prompt_session_state_started));

    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, prompt_session_state_change_callback, this);

    EXPECT_CALL(*this, prompt_session_state_change(_, mir_prompt_session_state_stopped));

    server_configuration.the_prompt_session_manager()->stop_prompt_session(server_prompt_session);

    // Verify we have got the "stopped" notification before we go on and release the session
    Mock::VerifyAndClearExpectations(the_mock_prompt_session_listener());

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, after_server_closes_prompt_session_api_isnt_broken)
{
    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    server_configuration.the_prompt_session_manager()->stop_prompt_session(server_prompt_session);

    EXPECT_FALSE(mir_prompt_session_add_prompt_provider_sync(prompt_session, existing_prompt_provider_pid));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, server_retrieves_application_session)
{
    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    EXPECT_THAT(server_configuration.the_prompt_session_manager()->application_for(server_prompt_session), Eq(application_session));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, server_retrieves_helper_session)
{
    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    // can get the helper session. but it will be the current pid.
    EXPECT_THAT(server_configuration.the_prompt_session_manager()->helper_for(server_prompt_session), SessionWithPid(getpid()));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, server_retrieves_existing_provider_sessions)
{
    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    mir_prompt_session_add_prompt_provider_sync(prompt_session, existing_prompt_provider_pid);
    mir_prompt_session_add_prompt_provider_sync(prompt_session, another_prompt_provider_pid);

    EXPECT_THAT(list_providers_for(server_prompt_session), ElementsAre(existing_prompt_provider_session, another_existing_prompt_provider));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, server_retrieves_child_provider_sessions)
{
    static int const no_of_prompt_providers = 2;
    char const* const child_provider_name[no_of_prompt_providers] =
    {
        "child_provider0",
        "child_provider1"
    };

    capture_server_prompt_session();

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    mir_wait_for(mir_prompt_session_new_fds_for_prompt_providers(prompt_session, no_of_prompt_providers, &client_fd_callback, this));

    DummyPromptProvider child_provider1{fd_connect_string(actual_fds[0]), child_provider_name[0]};
    DummyPromptProvider child_provider2{fd_connect_string(actual_fds[1]), child_provider_name[1]};

    EXPECT_THAT(list_providers_for(server_prompt_session), ElementsAre(SessionWithName(child_provider_name[0]), SessionWithName(child_provider_name[1])));

    mir_prompt_session_release_sync(prompt_session);
}

TEST_F(PromptSessionClientAPI, cannot_start_a_prompt_session_without_authorization)
{
    EXPECT_CALL(the_mock_session_authorizer(), prompt_session_is_allowed(_))
        .WillOnce(Return(false));

    // TODO is ugly releasing a connection so we can start one with chosen permissions
    mir_connection_release(connection);
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_CALL(*the_mock_prompt_session_listener(), starting(_)).Times(0);

    MirPromptSession* prompt_session = mir_connection_create_prompt_session_sync(
        connection, application_session_pid, null_state_change_callback, this);

    EXPECT_THAT(mir_prompt_session_is_valid(prompt_session), Eq(false));
    EXPECT_THAT(mir_prompt_session_error_message(prompt_session), HasSubstr("Prompt sessions disabled"));

    mir_prompt_session_release_sync(prompt_session);
}

