/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/graphics/renderer.h"
#include "mir_client/mir_client_library.h"
#include "mir/thread/all.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mtf = mir_test_framework;

namespace mir_test_framework
{
int const test_process = getpid();
std::atomic<int> client_pending_count(0);
std::atomic<int> client_connect_count(0);

void increment(std::atomic<int>& count)
{
    for (int new_count = count.load();
        !count.compare_exchange_weak(new_count, new_count + 1);
        std::this_thread::yield())
        ;
}

void send_connected_signal()
{
    sigqueue(test_process, SIGALRM, sigval());
}

extern "C"
{
static struct sigaction oldsigaction;
static struct sigaction newsigaction;

static void handle_connected_signal(int)
{
    increment(client_connect_count);
}
}
}

namespace
{
struct Client : TestingClientConfiguration
{
    void exec()
    {
        mir_wait_for(mir_connect(
            mtf::test_socket_file().c_str(),
            __PRETTY_FUNCTION__,
            connection_callback,
            this));

        mtf::send_connected_signal();

        MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            640, 480,
            mir_pixel_format_rgba_8888,
            mir_buffer_usage_hardware
        };

        mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));
        mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));
        mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));
        mir_wait_for(mir_surface_release( surface, release_surface_callback, this));

        mir_connection_release(connection);
    }

    Client() : connection(0), surface(0), buffers(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        Client* self = reinterpret_cast<Client*>(context);
        self->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }

    virtual void next_buffer(MirSurface*)
    {
        ++buffers;
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
    int buffers;
};

struct Server : TestingServerConfiguration
{
    std::shared_ptr<mg::Renderer> make_renderer(
        std::shared_ptr<mg::Display> const& /*display*/)
    {
        class StubRenderer : public mg::Renderer
        {
        public:
            virtual void render(mg::Renderable& /*r*/)
            {
                // NOOP will cause client threads to stall
            }
        };
        return std::make_shared<StubRenderer>();
    }
};
}

namespace mir_test_framework
{
struct FrontendShutdown : BespokeDisplayServerTestFixture
{
    static void SetUpTestCase()
    {
        BespokeDisplayServerTestFixture::SetUpTestCase();
        newsigaction.sa_handler = handle_connected_signal;
        newsigaction.sa_flags = SA_NODEFER;

        sigaction(SIGALRM, &newsigaction, &oldsigaction);
    }

    static void TearDownTestCase()
    {
        sigaction(SIGALRM, &oldsigaction, 0);
        BespokeDisplayServerTestFixture::TearDownTestCase();
    }

    void SetUp()
    {
        BespokeDisplayServerTestFixture::SetUp();
        client_pending_count.store(0);
        client_connect_count.store(0);
    }

    void TearDown()
    {
        BespokeDisplayServerTestFixture::TearDown();
    }

    Server server_processing;
    Client client_config;

    void launch_client_process(TestingClientConfiguration& config)
    {
        if (getpid() == test_process)
        {
            increment(client_pending_count);
            BespokeDisplayServerTestFixture::launch_client_process(config);
       }
    }

    void wait_for_client_to_connect()
    {
        if (getpid() == test_process)
        {
            // The following seems a but elaborate, but we sometimes
            // miss some notification signals.  In practice, after
            // 200*10ms the clients will be there.  Even under valgrind.
            int const max_clients = client_pending_count.load();
            int const min_clients = (max_clients / 3) + 1;

            for (auto retries = 0; (max_clients != client_connect_count.load()) && (retries != 200); ++retries)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (min_clients != client_connect_count.load())
            {
                std::cerr << "DEBUG: client_pending_count=" << client_pending_count.load() << std::endl;
                std::cerr << "DEBUG: client_connect_count=" << client_connect_count.load() << std::endl;
            }

            ASSERT_LE(min_clients, client_connect_count.load());
        }
    }
};
}
using mir_test_framework::FrontendShutdown;


TEST_F(FrontendShutdown, after_client_connects)
{
    launch_server_process(server_processing);

    launch_client_process(client_config);

    wait_for_client_to_connect();

    // The problem we're really testing for is a hang here
    // ...so there ought to be some sort of timeout.
    // Maybe that should be in Process::wait_for_termination()?
    EXPECT_TRUE(shutdown_server_process());

    // TODO: clients detect that server failed - and can exit themselves
    kill_client_processes();
}

TEST_F(FrontendShutdown, before_client_connects)
{
    launch_server_process(server_processing);

    // The problem we're really testing for is a hang here
    // ...so there ought to be some sort of timeout.
    // Maybe that should be in Process::wait_for_termination()?
    EXPECT_TRUE(shutdown_server_process());
}

TEST_F(FrontendShutdown, with_many_clients_connected)
{
    int const many = 3;

    launch_server_process(server_processing);

    for (int i = 0; i != many; ++i)
        launch_client_process(client_config);

    wait_for_client_to_connect();

    // The problem we're really testing for is a hang here
    // ...so there ought to be some sort of timeout.
    // Maybe that should be in Process::wait_for_termination()?
    EXPECT_TRUE(shutdown_server_process());

    // TODO: clients detect that server failed - and can exit themselves
    kill_client_processes();
}
