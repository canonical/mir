/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_trust_session.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

using MirClientLibraryTrustSessionTest = DefaultDisplayServerTestFixture;

namespace mir
{
namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(0)
        , trust_session(0)
        , started(0)
        , stopped(0)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    virtual void connected(MirConnection * new_connection)
    {
        connection = new_connection;
    }

    static void trust_session_start_callback(MirTrustSession * /* session */, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->started++;
    }

    static void trust_session_stop_callback(MirTrustSession * /* session */, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->stopped++;
    }


    MirConnection* connection;
    MirTrustSession* trust_session;
    int started;
    int stopped;
};
}

TEST_F(MirClientLibraryTrustSessionTest, client_library_trust_session)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ("", mir_connection_get_error_message(connection));

            trust_session = mir_connection_trust_session_create(connection);
            ASSERT_TRUE(trust_session != NULL);

            mir_wait_for(mir_trust_session_start(trust_session, trust_session_start_callback, this));
            EXPECT_EQ(started, 1);
            EXPECT_EQ(stopped, 0);

            mir_wait_for(mir_trust_session_stop(trust_session, trust_session_stop_callback, this));
            EXPECT_EQ(stopped, 1);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

}
