/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"

#include <mir_toolkit/mir_client_library.h>

#include "mir_test/fake_shared.h"
#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_framework/process.h"
#include "mir_test_doubles/stub_session_authorizer.h"
#include "mir/report_exception.h" // DEBUG

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace mir_test_framework
{
std::string const& test_socket_file(); // FRIG
}

namespace
{
struct ClientServerTest : mtf::HeadlessTest
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();

    pid_t test_process_id{getpid()};
    pid_t server_process_id{0};
    std::shared_ptr<mtf::Process> server_process;
    std::shared_ptr<mtf::Process> client_process;
    mtf::CrossProcessSync shutdown_sync;
    char const* process_tag = "test";

    ~ClientServerTest()
    {
        if (getpid() != test_process_id)
        {
            auto const status = ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") exiting status=" << status << std::endl;
//            if (getpid() == server_process_id) // FRIG
                exit(status);
        }
    }

    void run_server_with(std::function<void()> const& setup_code, std::function<void()> const& exec_code)
    {
        mtf::CrossProcessSync started_sync;

        pid_t pid = fork();

        if (pid < 0)
        {
            throw std::runtime_error("Failed to fork process");
        }

        if (pid == 0)
        {
            server_process_id = getpid();
            process_tag = "server";
            add_to_environment("MIR_SERVER_FILE", mir_test_socket);
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") pre-setup" << std::endl;
            setup_code();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") post-setup" << std::endl;
            start_server();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") running" << std::endl;
            started_sync.signal_ready();
            exec_code();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") exec done" << std::endl;
        }
        else
        {
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") waiting for server start..." << std::endl;
            server_process = std::make_shared<mtf::Process>(pid);
            started_sync.wait_for_signal_ready_for();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") ...continuing" << std::endl;
        }
    }

    void run_as_client(std::function<void()> const& client_code)
    {
        if (test_process_id != getpid()) return;

        pid_t pid = fork();

        if (pid < 0)
        {
            throw std::runtime_error("Failed to fork process");
        }

        if (pid == 0)
        {
            process_tag = "client";
            add_to_environment("MIR_SOCKET", mir_test_socket);
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") pre-client-code" << std::endl;
            client_code();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") post-client-code" << std::endl;
        }
        else
        {
            client_process = std::make_shared<mtf::Process>(pid);
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") waiting for client exit..." << std::endl;
            mtf::Result result = client_process->wait_for_termination();
            EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") client exit status=" << result.exit_code << std::endl;
        }
    }

    void TearDown() override
    {
        if (server_process_id == getpid())
            {
                std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") waiting for shutdown..." << std::endl;
                shutdown_sync.wait_for_signal_ready_for();
                std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") ...continuing to shutdown" << std::endl;
            }

        std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") TearDown" << std::endl;

        if (test_process_id != getpid()) return;

        std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") signalling shutdown..." << std::endl;
        shutdown_sync.signal_ready();

        if (server_process)
        {
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") terminating server" << std::endl;
            server_process->terminate();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") waiting for server" << std::endl;
            mtf::Result result = server_process->wait_for_termination();
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") server done" << std::endl;
            server_process.reset();
            EXPECT_THAT(result.exit_code, Eq(EXIT_SUCCESS));
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") server exit status=" << result.exit_code << std::endl;
        }
    }
};

struct MockSessionAuthorizer : public mf::SessionAuthorizer
{
    MOCK_METHOD1(connection_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(configure_display_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(screencast_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(prompt_session_is_allowed, bool(mf::SessionCredentials const&));
};

struct ClientCredsTestFixture : ClientServerTest
{
    ClientCredsTestFixture()
    {
        shared_region = static_cast<SharedRegion*>(
            mmap(NULL, sizeof(SharedRegion), PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, 0, 0));
        sem_init(&shared_region->client_creds_set, 1, 0);
    }

    struct SharedRegion
    {
        sem_t client_creds_set;
        pid_t client_pid;
        uid_t client_uid;
        gid_t client_gid;

        bool matches_client_process_creds(mf::SessionCredentials const& creds)
        {
            return client_pid == creds.pid() &&
                   client_uid == creds.uid() &&
                   client_gid == creds.gid();
        }

        bool wait_for_client_creds()
        {
            static time_t const timeout_seconds = 60;
            struct timespec abs_timeout = { time(NULL) + timeout_seconds, 0 };
            return sem_timedwait(&client_creds_set, &abs_timeout) == 0;
        }

        void post_client_creds()
        {
            client_pid = getpid();
            client_uid = geteuid();
            client_gid = getegid();
            sem_post(&client_creds_set);
        }
    };

    SharedRegion* shared_region;
    MockSessionAuthorizer mock_authorizer;
};
}

TEST_F(ClientCredsTestFixture, session_authorizer_receives_pid_of_connecting_clients)
{
    auto const server_setup = [&]
        {
            auto matches_creds = [&](mf::SessionCredentials const& creds)
            {
                return shared_region->matches_client_process_creds(creds);
            };

            EXPECT_CALL(mock_authorizer,
                connection_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(true));
            EXPECT_CALL(mock_authorizer,
                configure_display_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                screencast_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                prompt_session_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));

            server.override_the_session_authorizer([&] { return mt::fake_shared(mock_authorizer); });
        };

    auto const server_exec = [&]
        {
            EXPECT_TRUE(shared_region->wait_for_client_creds());
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") received creds:"
                "client pid=" << shared_region->client_pid << std::endl;

        };

    run_server_with(server_setup, server_exec);

    run_as_client([&]
        {
//    if (getpid() != server_process_id)
//    try
//    {
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") posting creds..." << std::endl;
            shared_region->post_client_creds();

            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") connecting..." << std::endl;
            auto const connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            ASSERT_TRUE(mir_connection_is_valid(connection));

            // Clear the lifecycle callback in order not to get SIGHUP by the
            // default lifecycle handler during connection teardown
            mir_connection_set_lifecycle_event_callback(
                connection,
                [](MirConnection*, MirLifecycleState, void*){},
                nullptr);

            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") disconnecting..." << std::endl;
            mir_connection_release(connection);
            std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") done" << std::endl;
//    }
//    catch (...)
//    {
//        mir::report_exception(std::cerr);
//        throw;
//    }
        });

    std::cerr << "DEBUG: " << process_tag << "("<< getpid() << ") end of test body" << std::endl;
}

#ifdef ARG_COMMENTED_OUT
// This test is also a regression test for https://bugs.launchpad.net/mir/+bug/1358191
TEST_F(ClientCredsTestFixture, authorizer_may_prevent_connection_of_clients)
{
    using namespace ::testing;

    struct ServerConfiguration : TestingServerConfiguration
    {
        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
        {
            struct StubAuthorizer : mtd::StubSessionAuthorizer
            {
                bool connection_is_allowed(mir::frontend::SessionCredentials const&) override
                {
                    return false;
                }
            };

            return std::make_shared<StubAuthorizer>();
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfiguration : TestingClientConfiguration
    {
        void exec() override
        {
            auto const connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            EXPECT_FALSE(mir_connection_is_valid(connection));
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
#endif
