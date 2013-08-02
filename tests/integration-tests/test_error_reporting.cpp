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


#include "mir_toolkit/mir_client_library.h"

#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/resource_cache.h"
#include "mir/frontend/communicator.h"

#include "mir_protobuf.pb.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/stub_ipc_factory.h"

#include <chrono>
#include <mutex>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

struct ErrorServer : mir::protobuf::DisplayServer
{
    static std::string const test_exception_text;

    void create_surface(
        google::protobuf::RpcController*,
        const mir::protobuf::SurfaceParameters*,
        mir::protobuf::Surface*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void release_surface(
        google::protobuf::RpcController*,
        const mir::protobuf::SurfaceId*,
        mir::protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }


    void connect(
        ::google::protobuf::RpcController*,
        const ::mir::protobuf::ConnectParameters*,
        ::mir::protobuf::Connection*,
        ::google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void disconnect(
        google::protobuf::RpcController*,
        const mir::protobuf::Void*,
        mir::protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void test_file_descriptors(
        google::protobuf::RpcController*,
        const mir::protobuf::Void*,
        mir::protobuf::Buffer*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }
};

std::string const ErrorServer::test_exception_text{"test exception text"};

struct SurfaceSync
{
    SurfaceSync() :
        surface(0)
    {
    }

    void surface_created(MirSurface * new_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = new_surface;
        wait_condition.notify_all();
    }

    void surface_released(MirSurface * /*released_surface*/)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = NULL;
        wait_condition.notify_all();
    }

    void wait_for_surface_create()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (!surface)
            wait_condition.wait(lock);
    }

    void wait_for_surface_release()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (surface)
            wait_condition.wait(lock);
    }


    std::mutex guard;
    std::condition_variable wait_condition;
    MirSurface * surface;
};

struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(NULL)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connected(connection);
    }

    void connected(MirConnection * new_connection)
    {
        std::unique_lock<std::mutex> lock(guard);
        connection = new_connection;
        wait_condition.notify_all();
    }

    void wait_for_connect()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (!connection)
            wait_condition.wait(lock);
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    MirConnection * connection;
    static const int max_surface_count = 5;
    SurfaceSync ssync[max_surface_count];
};
const int ClientConfigCommon::max_surface_count;

void create_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_created(surface);
}

void release_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_released(surface);
}

void wait_for_surface_create(SurfaceSync* context)
{
    context->wait_for_surface_create();
}

void wait_for_surface_release(SurfaceSync* context)
{
    context->wait_for_surface_release();
}
}

using ErrorReporting = BespokeDisplayServerTestFixture;

TEST_F(ErrorReporting, c_api_returns_error)
{

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mf::ProtobufIpcFactory> the_ipc_factory(
            std::shared_ptr<mir::frontend::Shell> const&,
            std::shared_ptr<mg::Display> const&,
            std::shared_ptr<mg::GraphicBufferAllocator> const&) override
        {
            static auto error_server = std::make_shared<ErrorServer>();
            return std::make_shared<mtd::StubIpcFactory>(*error_server);
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_FALSE(mir_connection_is_valid(connection));
            EXPECT_TRUE(std::strstr(mir_connection_get_error_message(connection), ErrorServer::test_exception_text.c_str()));

            MirSurfaceParameters const request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware
            };

            mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync);

            wait_for_surface_create(ssync);

            ASSERT_TRUE(ssync->surface != NULL);
            EXPECT_FALSE(mir_surface_is_valid(ssync->surface));
            EXPECT_TRUE(std::strstr(mir_surface_get_error_message(ssync->surface), ErrorServer::test_exception_text.c_str()));

            EXPECT_NO_THROW({
                MirSurfaceParameters response_params;
                mir_surface_get_parameters(ssync->surface, &response_params);
            });

            mir_surface_release(ssync->surface, release_surface_callback, ssync);

            wait_for_surface_release(ssync);

            ASSERT_TRUE(ssync->surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
