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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/frontend/session_authorizer.h"

#include <mir_toolkit/mir_client_library.h>

#include "mir_test/fake_shared.h"
#include "mir_test_framework/display_server_test_fixture.h"

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

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon() :
        connection(0),
        surface(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
};

struct ConnectingClient : ClientConfigCommon
{
    void exec()
    {
        mir_wait_for(mir_connect(
            mir_test_socket,
            __PRETTY_FUNCTION__,
            connection_callback,
            this));
         mir_connection_release(connection);
    }
};

struct ClientPidTestFixture : BespokeDisplayServerTestFixture
{
    ClientPidTestFixture()
    {
        shared_region = static_cast<SharedRegion*>(
            mmap(NULL, sizeof(SharedRegion), PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, 0, 0));
        sem_init(&shared_region->client_pid_set, 1, 0);
        shared_region->client_pid = 0;
    }

    struct SharedRegion 
    {
        sem_t client_pid_set;
        pid_t client_pid;

        pid_t get_client_process_pid()
        {
            static time_t const timeout_seconds = 60;
            struct timespec abs_timeout = { time(NULL) + timeout_seconds, 0 };
            sem_timedwait(&client_pid_set, &abs_timeout);
            return client_pid;
        }
        void post_client_process_pid(pid_t pid)
        {
            client_pid = pid;
            sem_post(&client_pid_set);
        }
    };
    
    SharedRegion* shared_region;
};

struct MockSessionAuthorizer : public mf::SessionAuthorizer
{
    MOCK_METHOD1(connection_is_allowed, bool(pid_t));
    MOCK_METHOD1(configure_display_is_allowed, bool(pid_t));
};

}

TEST_F(ClientPidTestFixture, session_authorizer_receives_pid_of_connecting_clients)
{
    struct ServerConfiguration : TestingServerConfiguration
    {
        ServerConfiguration(ClientPidTestFixture::SharedRegion *shared_region)
            : shared_region(shared_region)
        {
        }

        void exec() override
        {
            using namespace ::testing;
            pid_t client_pid = shared_region->get_client_process_pid();

            EXPECT_CALL(mock_authorizer, connection_is_allowed(client_pid)).Times(1)
                .WillOnce(Return(true));
        }
        
        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
        {
            return mt::fake_shared(mock_authorizer);
        }

        ClientPidTestFixture::SharedRegion* shared_region;
        MockSessionAuthorizer mock_authorizer;
    } server_config(shared_region);
    launch_server_process(server_config);
    

    struct ClientConfiguration : ConnectingClient
    {
        ClientConfiguration(ClientPidTestFixture::SharedRegion *shared_region)
            : shared_region(shared_region)
        {
        }

        void exec() override
        {
            pid_t client_pid = getpid();
            shared_region->post_client_process_pid(client_pid);

            ConnectingClient::exec();
        }

        ClientPidTestFixture::SharedRegion* shared_region;
    } client_config(shared_region);
    launch_client_process(client_config);    
}

TEST_F(ClientPidTestFixture, authorizer_may_prevent_connection_of_clients)
{
    using namespace ::testing;

    struct ServerConfiguration : TestingServerConfiguration
    {
        ServerConfiguration(ClientPidTestFixture::SharedRegion *shared_region)
            : shared_region(shared_region)
        {
        }

        void exec() override
        {
            using namespace ::testing;
            pid_t client_pid = shared_region->get_client_process_pid();

            EXPECT_CALL(mock_authorizer, connection_is_allowed(client_pid)).Times(1)
                .WillOnce(Return(false));
        }
        
        std::shared_ptr<mf::SessionAuthorizer> the_session_authorizer() override
        {
            return mt::fake_shared(mock_authorizer);
        }

        ClientPidTestFixture::SharedRegion* shared_region;
        MockSessionAuthorizer mock_authorizer;
    } server_config(shared_region);
    launch_server_process(server_config);
    

    struct ClientConfiguration : ConnectingClient
    {
        ClientConfiguration(ClientPidTestFixture::SharedRegion *shared_region)
            : shared_region(shared_region)
        {
        }

        void exec() override
        {
            pid_t client_pid = getpid();
            shared_region->post_client_process_pid(client_pid);

            mir_wait_for(mir_connect(
                mir_test_socket,
                __PRETTY_FUNCTION__,
                connection_callback,
                this));
            MirSurfaceParameters const parameters =
            {
                __PRETTY_FUNCTION__,
                1, 1,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };
            mir_connection_create_surface_sync(connection, &parameters);
            EXPECT_GT(strlen(mir_connection_get_error_message(connection)), static_cast<unsigned int>(0));
            mir_connection_release(connection);
        }

        ClientPidTestFixture::SharedRegion* shared_region;
    } client_config(shared_region);
    launch_client_process(client_config);    
}
