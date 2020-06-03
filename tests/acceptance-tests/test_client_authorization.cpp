/*
 * Copyright © 2013-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/test/fake_shared.h"
#include "mir_test_framework/interprocess_client_server_test.h"
#include "mir/test/doubles/stub_session_authorizer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>

namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
struct MockSessionAuthorizer : public mf::SessionAuthorizer
{
    MOCK_METHOD1(connection_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(configure_display_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(set_base_display_configuration_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(screencast_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(prompt_session_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(configure_input_is_allowed, bool(mf::SessionCredentials const&));
    MOCK_METHOD1(set_base_input_configuration_is_allowed, bool(mf::SessionCredentials const&));
};

struct ClientCredsTestFixture : mtf::InterprocessClientServerTest
{
    ClientCredsTestFixture()
    {
        shared_region = static_cast<SharedRegion*>(
            mmap(NULL, sizeof(SharedRegion), PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, 0, 0));
    }

    ~ClientCredsTestFixture()
    {
        munmap(shared_region, sizeof(SharedRegion));
    }

    struct SharedRegion
    {
        std::atomic<pid_t> client_pid{-1};
        std::atomic<uid_t> client_uid{0xFFFF};
        std::atomic<gid_t> client_gid{0xFFFF};

        bool matches_client_process_creds(mf::SessionCredentials const& creds)
        {
            // A perfect match is perfect. But sometimes it's not a perfect
            // match and that's OK too. Because fakeroot (used in deb builds)
            // returns 0 unconditionally, even for non-root users.
            return client_pid == creds.pid() &&
                   (client_uid == 0 || client_uid == creds.uid()) &&
                   (client_gid == 0 || client_gid == creds.gid());
        }

        void post_client_creds()
        {
            client_pid = getpid();
            client_uid = geteuid();
            client_gid = getegid();
        }
    };

    SharedRegion* shared_region;
    MockSessionAuthorizer mock_authorizer;
};
}

TEST_F(ClientCredsTestFixture, session_authorizer_receives_pid_of_connecting_clients)
{
    auto const matches_creds = [&](mf::SessionCredentials const& creds)
    {
        return shared_region->matches_client_process_creds(creds);
    };

    init_server([&]
        {
            server.override_the_session_authorizer(
                [&] { return mt::fake_shared(mock_authorizer); });

            EXPECT_CALL(mock_authorizer,
                connection_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(true));
            EXPECT_CALL(mock_authorizer,
                configure_display_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                prompt_session_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                configure_input_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                set_base_input_configuration_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(mock_authorizer,
                set_base_display_configuration_is_allowed(Truly(matches_creds)))
                .Times(1)
                .WillOnce(Return(false));
        });

    run_in_server([&]{});

    run_in_client([&]
        {
            shared_region->post_client_creds();

            auto const connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            ASSERT_TRUE(mir_connection_is_valid(connection));

            mir_connection_release(connection);
        });
}

// This test is also a regression test for https://bugs.launchpad.net/mir/+bug/1358191
TEST_F(ClientCredsTestFixture, authorizer_may_prevent_connection_of_clients)
{
    init_server([&]
        {
            server.override_the_session_authorizer(
                [&] { return mt::fake_shared(mock_authorizer); });

            EXPECT_CALL(mock_authorizer,
                connection_is_allowed(_))
                .Times(1)
                .WillOnce(Return(false));
        });

    run_in_server([&]{});

    run_in_client([&]
        {
            auto const connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
            EXPECT_FALSE(mir_connection_is_valid(connection));
            mir_connection_release(connection);
        });
}
