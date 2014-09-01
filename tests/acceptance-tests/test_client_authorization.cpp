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
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/stub_session_authorizer.h"

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

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

struct ClientCredsTestFixture : BespokeDisplayServerTestFixture
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
};

struct MockSessionAuthorizer : public mf::SessionAuthorizer
{
    MOCK_METHOD1(connection_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(configure_display_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(screencast_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(prompt_session_is_allowed, bool(mf::SessionCredentials const&));
};

}

TEST_F(ClientCredsTestFixture, session_authorizer_receives_pid_of_connecting_clients)
{
    mtf::CrossProcessSync connect_sync;

    struct ServerConfiguration : TestingServerConfiguration
    {
        ServerConfiguration(ClientCredsTestFixture::SharedRegion *shared_region,
                            mtf::CrossProcessSync const& connect_sync)
            : shared_region(shared_region),
              connect_sync(connect_sync)
        {
        }

        void on_start() override
        {
            using namespace ::testing;

            EXPECT_TRUE(shared_region->wait_for_client_creds());
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
            connect_sync.try_signal_ready_for();
        }

        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
        {
            return mt::fake_shared(mock_authorizer);
        }

        ClientCredsTestFixture::SharedRegion* shared_region;
        MockSessionAuthorizer mock_authorizer;
        mtf::CrossProcessSync connect_sync;
    } server_config(shared_region, connect_sync);
    launch_server_process(server_config);


    struct ClientConfiguration : TestingClientConfiguration
    {
        ClientConfiguration(ClientCredsTestFixture::SharedRegion *shared_region,
                            mtf::CrossProcessSync const& connect_sync)
            : shared_region(shared_region),
              connect_sync(connect_sync)
        {
        }

        void exec() override
        {
            shared_region->post_client_creds();
            connect_sync.wait_for_signal_ready_for();
            auto const connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            mir_connection_release(connection);
        }

        ClientCredsTestFixture::SharedRegion* shared_region;
        mtf::CrossProcessSync connect_sync;
    } client_config(shared_region, connect_sync);
    launch_client_process(client_config);
}

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
